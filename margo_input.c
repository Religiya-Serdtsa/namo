#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ttak/mem/mem.h>
#include <ttak/mem/epoch_gc.h>
#include <ttak/mem/detachable.h>
#include <ttak/mem/epoch.h>
#include <ttak/timing/timing.h>
typedef char *string;
#define auto __auto_type
#define MARGO_CAT_IMPL(a, b) a##b
#define MARGO_CAT(a, b) MARGO_CAT_IMPL(a, b)
#define MARGO_WEIRD_0(T) T
#define MARGO_WEIRD_1(T) T *
#define MARGO_WEIRD_2(T) T **
#define MARGO_WEIRD_3(T) T ***
#define MARGO_WEIRD_4(T) T ****
#define MARGO_WEIRD_5(T) T *****
#define MARGO_WEIRD_6(T) T ******
#define MARGO_WEIRD_7(T) T *******
#define MARGO_WEIRD_8(T) T ********
#define MARGO_WEIRD_SELECT(N) MARGO_CAT(MARGO_WEIRD_, N)
#define weird(T, N) MARGO_WEIRD_SELECT(N)(T)
typedef enum {
    MARGO_PRINT_KIND_BOOL,
    MARGO_PRINT_KIND_SIGNED,
    MARGO_PRINT_KIND_UNSIGNED,
    MARGO_PRINT_KIND_FLOAT,
    MARGO_PRINT_KIND_CHAR,
    MARGO_PRINT_KIND_STRING,
    MARGO_PRINT_KIND_POINTER,
} margo_print_kind_t;
typedef struct {
    margo_print_kind_t kind;
    union {
        bool boolean;
        long long s64;
        unsigned long long u64;
        double f64;
        const char *cstr;
        const void *ptr;
        char ch;
    } data;
} margo_print_value_t;
static inline margo_print_value_t margo_print_value_from_bool(bool value) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_BOOL, .data.boolean = value };
}
static inline margo_print_value_t margo_print_value_from_signed(long long value) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_SIGNED, .data.s64 = value };
}
static inline margo_print_value_t margo_print_value_from_unsigned(unsigned long long value) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_UNSIGNED, .data.u64 = value };
}
static inline margo_print_value_t margo_print_value_from_double(double value) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_FLOAT, .data.f64 = value };
}
static inline margo_print_value_t margo_print_value_from_long_double(long double value) {
    return margo_print_value_from_double((double)value);
}
static inline margo_print_value_t margo_print_value_from_char(char value) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_CHAR, .data.ch = value };
}
static inline margo_print_value_t margo_print_value_from_cstr(const char *text) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_STRING, .data.cstr = text };
}
static inline margo_print_value_t margo_print_value_from_pointer(const void *ptr) {
    return (margo_print_value_t){ .kind = MARGO_PRINT_KIND_POINTER, .data.ptr = ptr };
}
// string/size_t aliases resolve to the existing entries below, so avoid duplicates.
#define MARGO_PRINT_VALUE(value) _Generic((value), \
    bool: margo_print_value_from_bool, \
    char: margo_print_value_from_char, \
    signed char: margo_print_value_from_signed, \
    unsigned char: margo_print_value_from_unsigned, \
    short: margo_print_value_from_signed, \
    unsigned short: margo_print_value_from_unsigned, \
    int: margo_print_value_from_signed, \
    unsigned int: margo_print_value_from_unsigned, \
    long: margo_print_value_from_signed, \
    unsigned long: margo_print_value_from_unsigned, \
    long long: margo_print_value_from_signed, \
    unsigned long long: margo_print_value_from_unsigned, \
    float: margo_print_value_from_double, \
    double: margo_print_value_from_double, \
    long double: margo_print_value_from_long_double, \
    const char *: margo_print_value_from_cstr, \
    char *: margo_print_value_from_cstr, \
    default: margo_print_value_from_pointer \
)(value)
typedef struct {
    bool is_char;
    char ch;
    const char *text;
} margo_print_span_t;
static inline margo_print_span_t margo_print_span_from_char(long long value) {
    margo_print_span_t span = { .is_char = true, .ch = (char)value };
    return span;
}
static inline margo_print_span_t margo_print_span_from_cstr(const char *text) {
    margo_print_span_t span = { .is_char = false, .ch = 0, .text = text };
    return span;
}
// `_Generic` disallows duplicate compatible types; rely on pointer entries for aliases.
#define MARGO_PRINT_SPAN(value) _Generic((value), \
    char: margo_print_span_from_char, \
    signed char: margo_print_span_from_char, \
    unsigned char: margo_print_span_from_char, \
    short: margo_print_span_from_char, \
    unsigned short: margo_print_span_from_char, \
    int: margo_print_span_from_char, \
    unsigned int: margo_print_span_from_char, \
    long: margo_print_span_from_char, \
    unsigned long: margo_print_span_from_char, \
    long long: margo_print_span_from_char, \
    unsigned long long: margo_print_span_from_char, \
    const char *: margo_print_span_from_cstr, \
    char *: margo_print_span_from_cstr \
)(value)
static inline void margo_print_emit_span(margo_print_span_t span) {
    if (span.is_char) {
        fputc(span.ch, stdout);
    } else if (span.text) {
        fputs(span.text, stdout);
    }
}
static inline void margo_print_emit_value(const margo_print_value_t *value) {
    if (!value) {
        return;
    }
    switch (value->kind) {
        case MARGO_PRINT_KIND_BOOL:
            fputs(value->data.boolean ? "true" : "false", stdout);
            break;
        case MARGO_PRINT_KIND_SIGNED:
            fprintf(stdout, "%lld", value->data.s64);
            break;
        case MARGO_PRINT_KIND_UNSIGNED:
            fprintf(stdout, "%llu", value->data.u64);
            break;
        case MARGO_PRINT_KIND_FLOAT:
            fprintf(stdout, "%g", value->data.f64);
            break;
        case MARGO_PRINT_KIND_CHAR:
            fputc(value->data.ch, stdout);
            break;
        case MARGO_PRINT_KIND_STRING:
            if (value->data.cstr) {
                fputs(value->data.cstr, stdout);
            } else {
                fputs("(null)", stdout);
            }
            break;
        case MARGO_PRINT_KIND_POINTER:
            fprintf(stdout, "%p", value->data.ptr);
            break;
    }
}
static inline void margo_print_emit(const margo_print_value_t *values,
                                    size_t value_count,
                                    margo_print_span_t sep,
                                    bool has_sep,
                                    margo_print_span_t endl,
                                    bool has_endl) {
    margo_print_span_t active_sep = has_sep ? sep : margo_print_span_from_cstr(" ");
    margo_print_span_t active_endl = has_endl ? endl : margo_print_span_from_cstr("\n");
    for (size_t idx = 0; idx < value_count; ++idx) {
        if (idx > 0) {
            margo_print_emit_span(active_sep);
        }
        margo_print_emit_value(values + idx);
    }
    margo_print_emit_span(active_endl);
}
typedef struct {
    const char *fmt;
    void *ptr;
} margo_scan_param_t;
static inline margo_scan_param_t margo_scan_param_make(const char *fmt, void *ptr) {
    margo_scan_param_t param = { .fmt = fmt, .ptr = ptr };
    return param;
}
static inline margo_scan_param_t margo_scan_param_signed_char(signed char *ptr) {
    return margo_scan_param_make("%hhd", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsigned_char(unsigned char *ptr) {
    return margo_scan_param_make("%hhu", ptr);
}
static inline margo_scan_param_t margo_scan_param_short(short *ptr) {
    return margo_scan_param_make("%hd", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsigned_short(unsigned short *ptr) {
    return margo_scan_param_make("%hu", ptr);
}
static inline margo_scan_param_t margo_scan_param_int(int *ptr) {
    return margo_scan_param_make("%d", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsigned_int(unsigned int *ptr) {
    return margo_scan_param_make("%u", ptr);
}
static inline margo_scan_param_t margo_scan_param_long(long *ptr) {
    return margo_scan_param_make("%ld", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsigned_long(unsigned long *ptr) {
    return margo_scan_param_make("%lu", ptr);
}
static inline margo_scan_param_t margo_scan_param_long_long(long long *ptr) {
    return margo_scan_param_make("%lld", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsigned_long_long(unsigned long long *ptr) {
    return margo_scan_param_make("%llu", ptr);
}
static inline margo_scan_param_t margo_scan_param_size_t(size_t *ptr) {
    return margo_scan_param_make("%zu", ptr);
}
static inline margo_scan_param_t margo_scan_param_float(float *ptr) {
    return margo_scan_param_make("%f", ptr);
}
static inline margo_scan_param_t margo_scan_param_double(double *ptr) {
    return margo_scan_param_make("%lf", ptr);
}
static inline margo_scan_param_t margo_scan_param_long_double(long double *ptr) {
    return margo_scan_param_make("%Lf", ptr);
}
static inline margo_scan_param_t margo_scan_param_string(char *ptr) {
    return margo_scan_param_make("%s", ptr);
}
static inline margo_scan_param_t margo_scan_param_const_string(string ptr) {
    return margo_scan_param_make("%s", ptr);
}
static inline margo_scan_param_t margo_scan_param_unsupported(void *ptr) {
    (void)ptr;
    return margo_scan_param_make(NULL, NULL);
}
#define MARGO_SCAN_PARAM(value) _Generic((value), \
    signed char *: margo_scan_param_signed_char, \
    unsigned char *: margo_scan_param_unsigned_char, \
    short *: margo_scan_param_short, \
    unsigned short *: margo_scan_param_unsigned_short, \
    int *: margo_scan_param_int, \
    unsigned int *: margo_scan_param_unsigned_int, \
    long *: margo_scan_param_long, \
    unsigned long *: margo_scan_param_unsigned_long, \
    long long *: margo_scan_param_long_long, \
    unsigned long long *: margo_scan_param_unsigned_long_long, \
    size_t *: margo_scan_param_size_t, \
    float *: margo_scan_param_float, \
    double *: margo_scan_param_double, \
    long double *: margo_scan_param_long_double, \
    char *: margo_scan_param_string, \
    string: margo_scan_param_const_string, \
    default: margo_scan_param_unsupported \
)(value)
static inline int margo_scan_run(const margo_scan_param_t *params, size_t count, bool require_newline) {
    if (!params && count > 0) {
        return -1;
    }
    size_t matched = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!params[i].fmt) {
            return -1;
        }
        if (scanf(params[i].fmt, params[i].ptr) != 1) {
            return (int)matched;
        }
        matched++;
    }
    if (require_newline) {
        int ch = getchar();
        while (ch != '\n' && ch != EOF) {
            ch = getchar();
        }
    }
    return (int)matched;
}
static inline uint64_t margo_now_ticks(void) {
    return ttak_get_tick_count();
}
typedef struct margo_fallback_alloc_node {
    void *ptr;
    struct margo_fallback_alloc_node *next;
} margo_fallback_alloc_node_t;
static margo_fallback_alloc_node_t *margo_fallback_alloc_head = NULL;
static const size_t MARGO_DETACHABLE_MAX_BYTES = 256 * 1024;
#define MARGO_DETACHABLE_MAGIC 0x4D4152474F444554ULL
typedef struct {
    uint64_t magic;
    size_t requested;
    ttak_detachable_context_t *ctx;
    ttak_detachable_allocation_t allocation;
} margo_detachable_header_t;
typedef struct {
    pthread_mutex_t lock;
    bool initialized;
    ttak_detachable_context_t ctx;
} margo_detachable_runtime_t;
static margo_detachable_runtime_t margo_detachable_runtime = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false,
};
static inline ttak_detachable_context_t *margo_detachable_context(void) {
    if (margo_detachable_runtime.initialized) {
        return &margo_detachable_runtime.ctx;
    }
    pthread_mutex_lock(&margo_detachable_runtime.lock);
    if (!margo_detachable_runtime.initialized) {
        ttak_detachable_context_init(&margo_detachable_runtime.ctx,
            TTAK_ARENA_HAS_OWNER | TTAK_ARENA_HAS_EPOCH_RECLAMATION |
            TTAK_ARENA_HAS_DEFAULT_EPOCH_GC | TTAK_ARENA_USE_LOCKED_ACCESS);
        margo_detachable_runtime.initialized = true;
    }
    pthread_mutex_unlock(&margo_detachable_runtime.lock);
    return &margo_detachable_runtime.ctx;
}
static inline margo_detachable_header_t *margo_detachable_header_from_payload(const void *ptr) {
    if (!ptr) {
        return NULL;
    }
    margo_detachable_header_t *header = ((margo_detachable_header_t *)ptr) - 1;
    if (header->magic != MARGO_DETACHABLE_MAGIC) {
        return NULL;
    }
    return header;
}
static pthread_once_t margo_epoch_tls_once = PTHREAD_ONCE_INIT;
static pthread_key_t margo_epoch_tls_key;
static _Thread_local bool margo_epoch_registered = false;
static void margo_epoch_tls_cleanup(void *value) {
    (void)value;
    if (margo_epoch_registered) {
        ttak_epoch_deregister_thread();
        margo_epoch_registered = false;
    }
}
static void margo_epoch_tls_init(void) {
    pthread_key_create(&margo_epoch_tls_key, margo_epoch_tls_cleanup);
}
static inline void margo_epoch_ensure_registered(void) {
    if (margo_epoch_registered) {
        return;
    }
    pthread_once(&margo_epoch_tls_once, margo_epoch_tls_init);
    ttak_epoch_register_thread();
    pthread_setspecific(margo_epoch_tls_key, (void *)1);
    margo_epoch_registered = true;
}
static void margo_epoch_free_callback(void *ptr) {
    free(ptr);
}
static inline void margo_retire_fallback_ptr(void *ptr) {
    if (!ptr) {
        return;
    }
    margo_epoch_ensure_registered();
    ttak_epoch_retire(ptr, margo_epoch_free_callback);
    ttak_epoch_reclaim();
}
static inline bool margo_fallback_alloc_track(void *ptr) {
    if (!ptr) {
        return false;
    }
    margo_fallback_alloc_node_t *node = (margo_fallback_alloc_node_t *)malloc(sizeof(*node));
    if (!node) {
        errno = ENOMEM;
        return false;
    }
    node->ptr = ptr;
    node->next = margo_fallback_alloc_head;
    margo_fallback_alloc_head = node;
    return true;
}
static inline bool margo_fallback_alloc_untrack(void *ptr) {
    if (!ptr) {
        return false;
    }
    margo_fallback_alloc_node_t *prev = NULL;
    margo_fallback_alloc_node_t *cur = margo_fallback_alloc_head;
    while (cur) {
        if (cur->ptr == ptr) {
            if (prev) {
                prev->next = cur->next;
            } else {
                margo_fallback_alloc_head = cur->next;
            }
            free(cur);
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}
static inline void *margo_fallback_alloc(size_t bytes) {
    void *ptr = malloc(bytes);
    if (!ptr) {
        return NULL;
    }
    if (!margo_fallback_alloc_track(ptr)) {
        free(ptr);
        return NULL;
    }
    return ptr;
}
static inline void margo_builtin_free_ptr(void *ptr) {
    if (!ptr) {
        return;
    }
    margo_detachable_header_t *header = margo_detachable_header_from_payload(ptr);
    if (header) {
        ttak_detachable_context_t *ctx = header->ctx;
        ttak_detachable_allocation_t allocation = header->allocation;
        ttak_detachable_mem_free(ctx, &allocation);
        return;
    }
    if (margo_fallback_alloc_untrack(ptr)) {
        margo_retire_fallback_ptr(ptr);
        return;
    }
    free(ptr);
}
static inline void *margo_builtin_alloc_bytes(size_t bytes) {
    if (bytes == 0) {
        return NULL;
    }
    if (bytes > MARGO_DETACHABLE_MAX_BYTES) {
        return margo_fallback_alloc(bytes);
    }
    margo_epoch_ensure_registered();
    ttak_detachable_context_t *ctx = margo_detachable_context();
    if (ctx) {
        size_t total = bytes + sizeof(margo_detachable_header_t);
        ttak_detachable_allocation_t allocation = ttak_detachable_mem_alloc(ctx, total, margo_now_ticks());
        if (allocation.data) {
            margo_detachable_header_t *header = (margo_detachable_header_t *)allocation.data;
            header->magic = MARGO_DETACHABLE_MAGIC;
            header->requested = bytes;
            header->ctx = ctx;
            header->allocation = allocation;
            return (void *)(header + 1);
        }
    }
    return margo_fallback_alloc(bytes);
}
static inline void *margo_builtin_alloc_and_copy(size_t bytes, const void *src, size_t src_len) {
    void *dst = margo_builtin_alloc_bytes(bytes);
    if (dst && src && src_len) {
        size_t copy = src_len < bytes ? src_len : bytes;
        memcpy(dst, src, copy);
    }
    return dst;
}
static inline size_t margo_file_read(void *dst, size_t elem_size, size_t elem_count, FILE *stream) {
    if (!dst || !stream || elem_size == 0 || elem_count == 0) {
        return 0;
    }
    return fread(dst, elem_size, elem_count, stream);
}
static inline size_t margo_file_write(const void *src, size_t elem_size, size_t elem_count, FILE *stream) {
    if (!src || !stream || elem_size == 0 || elem_count == 0) {
        return 0;
    }
    return fwrite(src, elem_size, elem_count, stream);
}
static inline int margo_net_tcp_connect(const char *ipv4, uint16_t port) {
    if (!ipv4 || !*ipv4) {
        errno = EINVAL;
        return -1;
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ipv4, &addr.sin_addr) != 1) {
        close(sock);
        errno = EINVAL;
        return -1;
    }
    if (connect(sock, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
        int saved = errno;
        close(sock);
        errno = saved;
        return -1;
    }
    return sock;
}
static inline ssize_t margo_net_send(int sock, const void *buf, size_t len) {
    if (sock < 0 || (!buf && len > 0)) {
        errno = EINVAL;
        return -1;
    }
    return send(sock, buf, len, 0);
}
static inline ssize_t margo_net_recv(int sock, void *buf, size_t len) {
    if (sock < 0 || (!buf && len > 0)) {
        errno = EINVAL;
        return -1;
    }
    return recv(sock, buf, len, 0);
}
static inline void margo_net_close(int sock) {
    if (sock >= 0) {
        close(sock);
    }
}
#define alloc(size) margo_builtin_alloc_bytes((size_t)(size))
#define alloc_and_init(size, literal) margo_builtin_alloc_and_copy((size_t)(size), (literal), sizeof(literal))
#line 1 "main.margo"
// AUTO-GENERATED: flattened Margo translation unit

// ---- /home/yjlee/namo/estruct.margo ----
// estruct.margo
// Constants and Structures for namo

/* @style c */ 
#ifndef NULL
#include <stddef.h>
#endif
#ifndef NULL
#define NULL NULL
#endif
#include <wchar.h>
#include <ctype.h>
#undef isspace
extern int wcwidth(wchar_t c);
extern int isspace(int c);
extern  int getcmd();

const int MAXCOL = 512;
const int MAXROW = 1024;

const int NBINDS = 2048;
const int NFILEN = 2048;
const int NBUFN = 16;
const int NLINE = 2048;
const int NSTRING = 1024;
const int NKBDM = 2048;
const int NPAT = 1024;
const int HUGE = 1000;
const int NLOCKS = 1000;
const int NCOLORS = 8;
const int KBLOCK = 8192;

const int CONTROL = 0x10000000;
const int META = 0x20000000;
const int SHIFT = 0x08000000;
const int SUPER = 0x04000000;
const int CTLX = 0x40000000;
const int SPEC = 0x80000000;

const int FALSE = 0;
const int TRUE = 1;
const int ABORT = 2;
const int FAILED = 3;

const int STOP = 0;
const int PLAY = 1;
const int RECORD = 2;

const int FIOSUC = 0;
const int FIOFNF = 1;
const int FIOEOF = 2;
const int FIOERR = 3;
const int FIOMEM = 4;
const int FIONUL = 5;

const int GFREAD = 1;
const int PTBEG = 0;
const int PTEND = 1;
const int FORWARD = 0;
const int REVERSE = 1;
const int WFFORCE = 0x01;
const int WFMOVE = 0x02;
const int WFEDIT = 0x04;
const int WFHARD = 0x08;
const int WFMODE = 0x10;
const int WFCOLR = 0x20;
const int BFINVS = 0x01;
const int BFCHG = 0x02;
const int BFTRUNC = 0x04;
const int BFMAKE = 0x08;
const int MDWRAP = 0x0001;
const int MDSOFTWRAP = 0x0002;
const int MDCMOD = 0x0004;
const int MDSPELL = 0x0008;
const int MDEXACT = 0x0010;
const int MDVIEW = 0x0020;
const int MDOVER = 0x0040;
const int MDMAGIC = 0x0080;
const int MDASAVE = 0x0800;
const int CFCPCN = 0x0001;
const int CFKILL = 0x0002;

struct line;
typedef struct line line;

struct buffer;
typedef struct buffer buffer;

struct window;
typedef struct window window;

struct video;
typedef struct video video;

struct video_cell;
typedef struct video_cell video_cell;

struct terminal;
typedef struct terminal terminal;

struct region;
typedef struct region region;

struct key_tab;
typedef struct key_tab key_tab;

struct name_bind;
typedef struct name_bind name_bind;

struct video_cell {
    unsigned int ch; // unicode_t
    int fg;
    int bg;
    bool bold;
    bool underline;
    bool italic;
};

struct video {
    int v_flag;
    video_cell v_text[1];
};

const int VFCHG = 0x0001;
const int VFEXT = 0x0002;
const int VFREQ = 0x0008;
const int VFCOL = 0x0010;

struct kill {
    weird(struct kill, 1) d_next;
    char d_chunk[KBLOCK];
};

struct line {
    weird(struct line, 1) l_fp;
    weird(struct line, 1) l_bp;
    int l_size;
    int l_used;
    weird(void, 1) hl_start_state;
    weird(void, 1) hl_end_state;
    char l_text[1];
};

struct window {
    weird(struct buffer, 1) w_bufp;
    weird(struct line, 1) w_linep;
    weird(struct line, 1) w_dotp;
    weird(struct line, 1) w_markp;
    int w_doto;
    int w_marko;
    char w_force;
    char w_flag;
};

struct buffer {
    weird(struct buffer, 1) b_bufp;
    weird(struct line, 1) b_dotp;
    weird(struct line, 1) b_markp;
    weird(struct line, 1) b_linep;
    int b_doto;
    int b_marko;
    int b_mode;
    char b_active;
    char b_nwnd;
    char b_flag;
    char b_fname[NFILEN];
    char b_bname[NBUFN];
    int b_tabsize;
};

struct region {
    weird(struct line, 1) r_linep;
    int r_offset;
    long r_size;
};

struct terminal {
    int t_mrow;
    int t_nrow;
    int t_mcol;
    int t_ncol;
    int t_margin;
    int t_scrsiz;
    int t_pause;
    weird(void, 1) t_open;
    weird(void, 1) t_close;
    weird(void, 1) t_kopen;
    weird(void, 1) t_kclose;
    weird(void, 1) t_getchar;
    weird(void, 1) t_putchar;
    weird(void, 1) t_flush;
    weird(void, 1) t_move;
    weird(void, 1) t_eeol;
    weird(void, 1) t_eeop;
    weird(void, 1) t_beep;
    weird(void, 1) t_rev;
    weird(void, 1) t_italic;
    weird(void, 1) t_set_colors;
    weird(void, 1) t_set_attrs;
    weird(void, 1) t_rez;
};

struct key_tab {
    int k_code;
    weird(int, 1) k_fp;
};

struct name_bind {
    string n_name;
    weird(int, 1) n_func;
};

/* @style c */ {
extern void vttopen(void);
extern void vttclose(void);
extern void vttkopen(void);
extern void vttkclose(void);
extern int vttgetc(void);
extern int vttputc(int c);
extern void vttflush(void);
extern void vttmove(int row, int col);
extern void vtteeol(void);
extern void vtteeop(void);
extern void vttbeep(void);
extern void vttrev(int state);
extern void vttitalic(int state);
extern void vttsetcolors(int fg, int bg);
extern void vttsetattrs(int bold, int underline, int italic);
extern int vttrez(char *res);

#define TTopen      vttopen
#define TTclose     vttclose
#define TTkopen     vttkopen
#define TTkclose    vttkclose
#define TTgetc      vttgetc
#define TTputc      vttputc
#define TTflush     vttflush
#define TTmove      vttmove
#define TTeeol      vtteeol
#define TTeeop      vtteeop
#define TTbeep      vttbeep
#define TTrev       vttrev
#define TTitalic    vttitalic
#define TTsetcolors vttsetcolors
#define TTsetattrs  vttsetattrs
#define TTrez       vttrez

extern void mlwrite(const char *fmt, ...);
extern void mlforce(const char *s, ...);
}

 int isletter(int c) {
    auto ch = c & 0xFF;
    if (ch >= (int)'a' && ch <= (int)'z') return TRUE;
    if (ch >= (int)'A' && ch <= (int)'Z') return TRUE;
    return FALSE;
}

extern  void mlerase();
extern  void upmode();
extern  int mlreplyt(string prompt, weird(char, 1) buf, int len, int metac);
extern  int minibuf_input(string prompt, weird(char, 1) buf, int len);
extern  int mlyesno(string prompt);
extern  int boundry(weird(struct line, 1) lp, int off, int dir);
extern  int rdonly();
extern  void kdelete();
extern  int backchar(int f, int n);
extern  int forwchar(int f, int n);
extern  int linsert(int n, int c);
extern  int ldelchar(long n, int f);
extern  int ldelete(long n, int f);
extern  int linstr(string s);
extern  int lover(string s);
extern  void TTbeep();
extern  int next_tab_stop(int col, int tab_width_val);
extern  int adjustmode(int kind, int global);
extern  weird(struct buffer, 1) bfind(string bname, int cflag, int bflag);
extern  int swbuffer(weird(struct buffer, 1) bp);
extern  int readin(string fname, int f);
extern  int zotbuf(weird(struct buffer, 1) bp);
extern  int bclear(weird(struct buffer, 1) bp);
extern  void lputc(weird(struct line, 1) lp, int n, int c);
extern  weird(struct line, 1) lalloc(int used);
extern  void lfree(weird(struct line, 1) lp);
extern  int killregion(int f, int n);
extern  int getregion(weird(struct region, 1) rp);
extern  void gotobob(int f, int n);
extern  void gotoeob(int f, int n);
extern  void gotobop(int f, int n);
extern  void gotoeop(int f, int n);
extern  int lnewline();
extern  int lgetchar(weird(unsigned int, 1) cp);
extern  void namo_handle_closed_file(string fname);

// ---- /home/yjlee/namo/globals.margo ----

int fillcol = 72;
string execstr = NULL;
char golabel[NPAT];
int execlevel = 0;
int eolexist = TRUE;
int revexist = FALSE;
int flickcode = FALSE;

const int MODE_NAME_COUNT = 9;
const int DNAME_COUNT = 10;
string modename[MODE_NAME_COUNT];
string mode2name[MODE_NAME_COUNT];
string dname[DNAME_COUNT];
int globals_tables_inited = FALSE;

 void globals_init_tables() {
    if (globals_tables_inited) return;
    modename[0] = "WRAP";
    modename[1] = "CMODE";
    modename[2] = "SPELL";
    modename[3] = "EXACT";
    modename[4] = "VIEW";
    modename[5] = "OVER";
    modename[6] = "MAGIC";
    modename[7] = "ASAVE";
    modename[8] = "UTF-8";

    mode2name[0] = "Wrap";
    mode2name[1] = "Cmode";
    mode2name[2] = "Spell";
    mode2name[3] = "Exact";
    mode2name[4] = "View";
    mode2name[5] = "Over";
    mode2name[6] = "Magic";
    mode2name[7] = "Asave";
    mode2name[8] = "utf-8";

    dname[0] = "if";
    dname[1] = "else";
    dname[2] = "endif";
    dname[3] = "goto";
    dname[4] = "return";
    dname[5] = "endm";
    dname[6] = "while";
    dname[7] = "endwhile";
    dname[8] = "break";
    dname[9] = "force";

    globals_tables_inited = TRUE;
}

char modecode[] = "WCSEVOMYAU";
int gmode = 0;
int gflags = GFREAD;
int gfcolor = 7;
int gbcolor = 0;
int gasave = 256;
int gacount = 256;
int sgarbf = TRUE;
int mpresf = FALSE;
int clexec = FALSE;
int mstore = FALSE;
int discmd = TRUE;
int disinp = TRUE;

weird(struct buffer, 1) bstore = NULL;
int vtrow = 0;
int vtcol = 0;
int ttrow = HUGE;
int ttcol = HUGE;
int lbound = 0;
int taboff = 0;
int metac = CONTROL | (int)'[';
int ctlxc = CONTROL | (int)'X';
int reptc = CONTROL | (int)'U';
int abortc = 0x1F;

int quotec = 0x11;
int tab_width = 7;
weird(struct kill, 1) kbufp = NULL;
weird(struct kill, 1) kbufh = NULL;
int kused = 0;
weird(struct window, 1) swindow = NULL;
int kbdmode = STOP;
int kbdrep = 0;
int restflag = FALSE;
int lastkey = 0;
int seed = 0;
long envram = 0L;
int macbug = FALSE;
int cmdstatus = TRUE;
char palstr[49] = "";
int saveflag = 0;
string fline = NULL;
int flen = 0;
int rval = 0;
int nullflag = FALSE;

weird(struct terminal, 1) term = NULL;

int justflag = FALSE;
int overlap = 0;
int scrollcount = 1;

int currow = 0;
int curcol = 0;
int thisflag = 0;
int lastflag = 0;
int curgoal = 0;
weird(struct window, 1) curwp = NULL;
weird(struct buffer, 1) curbp = NULL;
weird(struct buffer, 1) bheadp = NULL;
weird(struct buffer, 1) blistp = NULL;

char pat[NPAT];
char tap[NPAT];
char rpat[NPAT];

string patmatch = NULL;
weird(struct line, 1) matchline = NULL;
int matchoff = 0;
int cutln_active = FALSE;
int confirmshell = TRUE;
int makebackup = TRUE;
int removebackup = FALSE;

weird(struct line, 1) indent_start_lp = NULL;
weird(struct line, 1) indent_end_lp = NULL;
int indent_range_type = 0;
int indent_selection_active = FALSE;

int kbdm[NKBDM];
weird(int, 1) kbdptr = NULL;
weird(int, 1) kbdend = NULL;

unsigned int matchlen = 0;
unsigned int mlenold = 0;

string errorm = "ERROR";
string truem = "TRUE";
string falsem = "FALSE";

// ---- /home/yjlee/namo/utf8.margo ----
#include <wchar.h>

 unsigned int utf8_to_unicode(weird(unsigned char, 1) line, unsigned int index, unsigned int len, weird(unsigned int, 1) res) {
    if (line == NULL || res == NULL || index >= len) return 0;

    auto c = line[index];
    res[0] = c;

    if (c < 0x80) return 1;
    if ((c & 0xC0) == 0x80) return 1;

    auto available = len - index;
    if ((c & 0xE0) == 0xC0) {
        if (available < 2) goto invalid;
        auto c1 = line[index + 1];
        if ((c1 & 0xC0) != 0x80) goto invalid;
        auto value = ((unsigned int)(c & 0x1F) << 6) | (unsigned int)(c1 & 0x3F);
        if (value < 0x80) goto invalid;
        res[0] = value;
        return 2;
    }

    if ((c & 0xF0) == 0xE0) {
        if (available < 3) goto invalid;
        auto c1 = line[index + 1];
        auto c2 = line[index + 2];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) goto invalid;
        auto value = ((unsigned int)(c & 0x0F) << 12) | ((unsigned int)(c1 & 0x3F) << 6) | (unsigned int)(c2 & 0x3F);
        if (value < 0x800) goto invalid;
        if (value >= 0xD800 && value <= 0xDFFF) goto invalid;
        res[0] = value;
        return 3;
    }

    if ((c & 0xF8) == 0xF0) {
        if (available < 4) goto invalid;
        auto c1 = line[index + 1];
        auto c2 = line[index + 2];
        auto c3 = line[index + 3];
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) goto invalid;
        auto value = ((unsigned int)(c & 0x07) << 18) | ((unsigned int)(c1 & 0x3F) << 12) | ((unsigned int)(c2 & 0x3F) << 6) | (unsigned int)(c3 & 0x3F);
        if (value < 0x10000 || value > 0x10FFFF) goto invalid;
        res[0] = value;
        return 4;
    }

invalid:;
    res[0] = 0xFFFD;
    return 1;
}

 void reverse_string(weird(unsigned char, 1) begin, weird(unsigned char, 1) end) {
    while (begin < end) {
        auto a = begin[0];
        auto b = end[0];
        end[0] = a;
        begin[0] = b;
        begin++;
        end--;
    }
}

 unsigned int unicode_to_utf8(unsigned int c, weird(unsigned char, 1) utf8) {
    auto bytes = 1;
    utf8[0] = (unsigned char)c;
    if (c > 0x7f) {
        auto prefix = 0x40;
        auto p = utf8;
        while (c >= prefix) {
            p[0] = 0x80 + (unsigned char)(c & 0x3f);
            p++;
            bytes++;
            prefix >>= 1;
            c >>= 6;
        }
        p[0] = (unsigned char)(c - 2 * prefix);
        reverse_string(utf8, p);
    }
    return bytes;
}

 int unicode_width(unsigned int c) {
    if (c < 0x20 || c == 0x7F) return 2;
    if (c >= 0x80 && c <= 0xA0) return 3;
    auto width = wcwidth((wchar_t)c);
    if (width < 0) return 1;
    return width;
}

 bool is_beginning_utf8(unsigned char c) {
    return (c & 0xc0) != 0x80;
}

// ---- /home/yjlee/namo/util.margo ----

/* @style c */ {
    static inline FILE *namo_c_fopen(char *path, char *mode) {
        return fopen(path, mode);
    }
}

extern weird(FILE, 1) namo_c_fopen(string path, string mode);

 void mystrscpy(weird(char, 1) dst, string src, int size) {
    if (size == 0) { return; }
    auto i = 0;
    while (i < size - 1) {
        auto c = (int)src[i];
        if (c == 0) { break; }
        dst[i] = (char)c;
        i++;
    }
    dst[i] = (char)0;
}

 int mystrnlen_raw_w(unsigned int c) {
    if (c >= 0x4E00 && c <= 0x9FFF) { return 2; }
    if (c >= 0xAC00 && c <= 0xD7AF) { return 2; }
    if (c >= 0x3040 && c <= 0x309F) { return 2; }
    if (c >= 0x30A0 && c <= 0x30FF) { return 2; }
    return unicode_width(c);
}

 int next_tab_stop(int col, int tab_width_val) {
    auto step = tab_width_val + 1;
    if (step == 0) { step = 1; };
    return col - (col % step) + step;
}

 int next_column(int old, unsigned int c, int tab_width) {
    if (c == (unsigned int)9) { return next_tab_stop(old, tab_width); } // 9 is '\t'
    return old + mystrnlen_raw_w(c);
}

 int utf8_display_width(string str, int byte_len) {
    auto i = 0;
    auto width = 0;
    while (i < byte_len && 0 != (int)str[i]) {
        unsigned int c = 0;
        auto bytes = (int)utf8_to_unicode((weird(unsigned char, 1))str, (unsigned int)i, (unsigned int)byte_len, (weird(unsigned int, 1))(&c));
        if (bytes <= 0) { break; }
        width += mystrnlen_raw_w(c);
        i += bytes;
    }
    return width;
}

 int spellcheck(string word) {
    return FALSE;
}

// ---- /home/yjlee/namo/version.margo ----
#include <stdio.h>

const string PROGRAM_NAME = "namo";
const string PROGRAM_NAME_LONG = "namo editor";
const string VERSION = "0.0.1";

 void version() {
    margo_print_emit((margo_print_value_t[]){MARGO_PRINT_VALUE(PROGRAM_NAME_LONG), MARGO_PRINT_VALUE("version"), MARGO_PRINT_VALUE(VERSION)}, 3, (margo_print_span_t){0}, false, (margo_print_span_t){0}, false);
}

// ---- /home/yjlee/namo/usage.margo ----
#include <stdlib.h>
#include <stdio.h>

 void die(string err) {
    margo_print_emit((margo_print_value_t[]){MARGO_PRINT_VALUE("fatal:"), MARGO_PRINT_VALUE(err)}, 2, (margo_print_span_t){0}, false, (margo_print_span_t){0}, false);
    exit(128);
}

// ---- /home/yjlee/namo/wrapper.margo ----
#include <stdlib.h>
#include <stdio.h>

 int xmkstemp(string template) {
    auto fd = mkstemp(template);
    if (fd < 0) {
        die("Unable to create temporary file");
    }
    return fd;
}

 weird(void, 1) xmalloc(int size) {
    auto ret = alloc(size);
    if (ret == NULL) {
        die("Out of memory");
    }
    return ret;
margo_builtin_free_ptr(ret);
}

// ---- /home/yjlee/namo/fileio.margo ----
#include <stdio.h>
#include <string.h>

weird(FILE, 1) ffp = NULL;
int eofflag = FALSE;

 int ffropen(string fname) {
    ffp = namo_c_fopen(fname, "r");
    if (ffp == NULL) { return FIOFNF; }
    eofflag = FALSE;
    return FIOSUC;
}

 int ffwopen(string fname) {
    ffp = namo_c_fopen(fname, "w");
    if (ffp == NULL) {
        mlwrite("Cannot open file for writing");
        return FIOERR;
    }
    return FIOSUC;
}

 int ffclose() {
    if (fline != NULL) {
        free(fline);
        fline = NULL;
    }
    eofflag = FALSE;
    if (fclose(ffp) != 0) {
        mlwrite("Error closing file");
        return FIOERR;
    }
    return FIOSUC;
}

 int ffputline(string buf, int nbuf) {
    for (auto i=0; i <nbuf; ++i) {
        fputc((int)buf[i] & 0xFF, ffp);
    }
    fputc(10, ffp); // '\n'
    if (ferror(ffp) != 0) {
        mlwrite("Write I/O error");
        return FIOERR;
    }
    return FIOSUC;
}

 int ffgetline() {
    if (eofflag) { return FIOEOF; }

    if (flen > NSTRING) {
        free(fline);
        fline = NULL;
    }

    if (fline == NULL) {
        flen = NSTRING;
        fline = alloc(flen);
        if (fline == NULL) { margo_builtin_free_ptr(fline);
return FIOMEM; }
    margo_builtin_free_ptr(fline);
}

    auto i = 0;
    auto c = 0;

    if (nullflag == 0) {
        if (fgets(fline, NSTRING, ffp) == NULL) {
            i = 0;
            c = -1; // EOF
        } else {
            i = (int)strlen(fline);
            c = 0;
            if (i > 0) {
                c = (int)fline[i-1];
                if (c == 10 || c == 13) { // '\n' || '\r'
                    i--;
                }
            }
        }
    } else {
        i = 0;
        c = fgetc(ffp);
    }

    while (c != -1 && c != 10) { // -1 is EOF, 10 is '\n'
        if (c != 0 && c != 13) { // 13 is '\r'
            fline[i] = (char)c;
            i++;
            if (i >= flen) {
                auto tmpline = alloc(flen + NSTRING);
                if (tmpline == NULL) { margo_builtin_free_ptr(tmpline);
return FIOMEM; }
                memcpy(tmpline, fline, (size_t)flen);
                flen += NSTRING;
                free(fline);
                fline = tmpline;
            margo_builtin_free_ptr(tmpline);
}
        }
        c = fgetc(ffp);
    }

    if (c == -1) {
        if (ferror(ffp) != 0) {
            mlwrite("File read error");
            return FIOERR;
        }
        if (i != 0) {
            eofflag = TRUE;
        } else {
            return FIOEOF;
        }
    }

    fline[i] = (char)0;
    return FIOSUC;
}

 int fexist(string fname) {
    auto fp = namo_c_fopen(fname, "r");
    if (fp == NULL) { return FALSE; }
    fclose(fp);
    return TRUE;
}

// ---- /home/yjlee/namo/line.margo ----
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int BLOCK_SIZE = 16;

extern  int backchar(int f, int n);
extern  int rdonly();
extern  int linsert(int n, int c);
extern  int lnewline();
extern  int lgetchar(weird(unsigned int, 1) c);
extern  int ldelete(long n, int kflag);
extern  int ldelnewline();
extern  int kinsert(int c);
extern  int killtext(int f, int n);
extern  int backline(int f, int n);

 weird(struct line, 1) lforw(weird(struct line, 1) lp) { return lp->l_fp; }
 weird(struct line, 1) lback(weird(struct line, 1) lp) { return lp->l_bp; }
 int lgetc(weird(struct line, 1) lp, int n) { return (int)lp->l_text[n] & 0xFF; }
 void lputc(weird(struct line, 1) lp, int n, int c) { lp->l_text[n] = (char)c; };
 int llength(weird(struct line, 1) lp) { return lp->l_used; }

 weird(struct line, 1) lalloc(int used) {
    auto size = (used + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
    if (0 == size) {
        size = BLOCK_SIZE;
    }
    
    auto lp = (weird(struct line, 1))alloc(sizeof(line) + (size_t)size);
    if (NULL == lp) {
        mlwrite("(OUT OF MEMORY)");
        return NULL;
    }
    lp->l_size = size;
    lp->l_used = used;
    return lp;
}

 void lfree(weird(struct line, 1) lp) {
    auto wp_l = curwp;
    if (lp == wp_l->w_linep) {
        wp_l->w_linep = lp->l_fp;
    }
    if (lp == wp_l->w_dotp) {
        wp_l->w_dotp = lp->l_fp;
        wp_l->w_doto = 0;
    }
    if (lp == wp_l->w_markp) {
        wp_l->w_markp = lp->l_fp;
        wp_l->w_marko = 0;
    }

    auto bp = bheadp;
    while (NULL != bp) {
        if (0 == bp->b_nwnd) {
            if (lp == bp->b_dotp) {
                bp->b_dotp = lp->l_fp;
                bp->b_doto = 0;
            }
            if (lp == bp->b_markp) {
                bp->b_markp = lp->l_fp;
                bp->b_marko = 0;
            }
        }
        bp = bp->b_bufp;
    }
    lp->l_bp->l_fp = lp->l_fp;
    lp->l_fp->l_bp = lp->l_bp;
    free(lp);
}

 void lchange(int flag) {
    if (1 != curbp->b_nwnd) {
        flag = WFHARD;
    }
    if (0 == (curbp->b_flag & BFCHG)) {
        flag |= WFMODE;
        curbp->b_flag |= BFCHG;
    }
    auto wp_l = curwp;
    if (curbp == wp_l->w_bufp) {
        wp_l->w_flag = (char)((int)wp_l->w_flag | flag);
    }
}

 int insspace(int f, int n) {
    linsert(n, 32); // space
    backchar(f, n);
    return TRUE;
}

 int linstr(string instr) {
    auto status = TRUE;
    if (NULL != instr) {
        auto i = 0;
        while (0 != instr[i] && status == TRUE) {
            auto tmpc = (int)instr[i];
            if (13 == tmpc) { // '\r'
                status = lnewline();
                if (10 == (int)instr[i+1]) { // '\n'
                    i++;
                }
            } else if (10 == tmpc) { // '\n'
                status = lnewline();
            } else {
                status = linsert(1, tmpc);
            }
            if (TRUE != status) {
                mlwrite("%%Out of memory while inserting");
                break;
            }
            i++;
        }
    }
    return status;
}

 int linsert_byte(int n, int c) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    lchange(WFEDIT);
    auto lp1 = curwp->w_dotp;
    if (curbp->b_linep == lp1) {
        if (0 != curwp->w_doto) {
            mlwrite("bug: linsert");
            return FALSE;
        }
        auto lp2 = lalloc(n);
        if (NULL == lp2) { return FALSE; }
        auto lp3 = lp1->l_bp;
        lp3->l_fp = lp2;
        lp2->l_fp = lp1;
        lp1->l_bp = lp2;
        lp2->l_bp = lp3;
        for (auto i=0; i <n; ++i) {
            lp2->l_text[i] = (char)c;
        }
        curwp->w_dotp = lp2;
        curwp->w_doto = n;
        return TRUE;
    }
    auto doto = curwp->w_doto;
    auto lp2 = (weird(struct line, 1))NULL;
    if (lp1->l_used + n > lp1->l_size) {
        lp2 = lalloc(lp1->l_used + n);
        if (NULL == lp2) { return FALSE; }
        memcpy(lp2->l_text, lp1->l_text, (size_t)doto);
        memcpy(lp2->l_text + doto + n, lp1->l_text + doto, (size_t)(lp1->l_used - doto));
        lp1->l_bp->l_fp = lp2;
        lp2->l_fp = lp1->l_fp;
        lp1->l_fp->l_bp = lp2;
        lp2->l_bp = lp1->l_bp;
        free(lp1);
    } else {
        lp2 = lp1;
        memmove(lp1->l_text + doto + n, lp1->l_text + doto, (size_t)(lp1->l_used - doto));
        lp2->l_used += n;
    }
    for (auto i=0; i <n; ++i) {
        lp2->l_text[doto + i] = (char)c;
    }
    auto wp_l = curwp;
    if (lp1 == wp_l->w_linep) {
        wp_l->w_linep = lp2;
    }
    if (lp1 == wp_l->w_dotp) {
        wp_l->w_dotp = lp2;
        wp_l->w_doto += n;
    }
    if (lp1 == wp_l->w_markp) {
        wp_l->w_markp = lp2;
        if (wp_l->w_marko > doto) {
            wp_l->w_marko += n;
        }
    }
    return TRUE;
}

 int linsert(int n, int c) {
    char utf8[6];
    auto bytes = (int)unicode_to_utf8((unsigned int)c, (weird(unsigned char, 1))utf8);
    if (1 == bytes) { return linsert_byte(n, (int)utf8[0]); }
    for (auto i=0; i <n; ++i) {
        for (auto j=0; j <bytes; ++j) {
            if (FALSE == linsert_byte(1, (int)utf8[j])) { return FALSE; }
        }
    }
    return TRUE;
}

 int sanitize_and_insert(int n, int c) {
    if (10 == c || 9 == c || 13 == c) { // \n, \t, \r
        if (10 == c) {
            while (n > 0) {
                if (FALSE == lnewline()) { return FALSE; }
                n--;
            }
            return TRUE;
        }
        return linsert(n, c);
    } else {
        if (127 == c || (c < 32 && c >= 0)) {
            return TRUE;
        } else {
            return linsert(n, c);
        }
    }
}

 int lowrite(int c) {
    if (curwp->w_doto < curwp->w_dotp->l_used) {
        unsigned int existing_char = 0;
        auto bytes = lgetchar((weird(unsigned int, 1))(&existing_char));
        if (9 != (int)existing_char || (curwp->w_doto & tab_width) == tab_width) {
            ldelete((long)bytes, FALSE);
        }
    }
    return linsert(1, c);
}

 int lover(string ostr) {
    auto status = TRUE;
    if (NULL != ostr) {
        auto i = 0;
        while (0 != ostr[i] && status == TRUE) {
            auto tmpc = (int)ostr[i];
            if (13 == tmpc) { // \r
                status = lnewline();
                if (10 == (int)ostr[i+1]) { // \n
                    i++;
                }
            } else if (10 == tmpc) { // \n
                status = lnewline();
            } else {
                status = lowrite(tmpc);
            }
            if (TRUE != status) {
                mlwrite("%%Out of memory while overwriting");
                break;
            }
            i++;
        }
    }
    return status;
}

 int lnewline() {
    auto lp1 = curwp->w_dotp;
    auto doto = curwp->w_doto;
    lchange(WFHARD);

    if (curbp->b_linep == lp1) {
        if (0 != doto) {
            mlwrite("bug: lnewline at sentinel");
            return FALSE;
        }
        auto lp2 = lalloc(0);
        if (NULL == lp2) { return FALSE; }
        auto lp3 = lp1->l_bp;
        lp3->l_fp = lp2;
        lp2->l_fp = lp1;
        lp1->l_bp = lp2;
        lp2->l_bp = lp3;
        return TRUE;
    }

    while (doto > 0 && doto < lp1->l_used && 0 == is_beginning_utf8((unsigned char)lp1->l_text[doto])) {
        doto--;
    }

    auto lp2 = lalloc(lp1->l_used - doto);
    if (NULL == lp2) { return FALSE; }

    memcpy(lp2->l_text, lp1->l_text + doto, (size_t)(lp1->l_used - doto));

    lp2->l_fp = lp1->l_fp;
    lp1->l_fp = lp2;
    lp2->l_fp->l_bp = lp2;
    lp2->l_bp = lp1;
    lp1->l_used = doto;

    auto wp_l = curwp;
    if (lp1 == wp_l->w_dotp) {
        if (wp_l->w_doto >= doto) {
            wp_l->w_dotp = lp2;
            wp_l->w_doto -= doto;
        }
    }
    if (lp1 == wp_l->w_markp) {
        if (wp_l->w_marko > doto) {
            wp_l->w_markp = lp2;
            wp_l->w_marko -= doto;
        }
    }
    return TRUE;
}

 int lgetchar(weird(unsigned int, 1) c) {
    auto len = curwp->w_dotp->l_used;
    auto buf = curwp->w_dotp->l_text;
    return (int)utf8_to_unicode((weird(unsigned char, 1))buf, (unsigned int)curwp->w_doto, (unsigned int)len, c);
}

 int ldelchar(long n, int kflag) {
    while (n > 0L) {
        unsigned int c = 0;
        auto bytes = lgetchar((weird(unsigned int, 1))(&c));
        if (bytes <= 0) { return FALSE; }
        if (TRUE != ldelete((long)bytes, kflag)) { return FALSE; }
        n--;
    }
    lchange(WFHARD);
    return TRUE;
}

 int ldelete(long n, int kflag) {
    while (n != 0L) {
        auto dotp = curwp->w_dotp;
        auto doto = curwp->w_doto;
        if (curbp->b_linep == dotp) { return FALSE; }
        auto chunk = dotp->l_used - doto;
        auto long_chunk = (long)chunk;
        if (long_chunk > n) {
            chunk = (int)n;
        }
        if (0 == chunk) {
            lchange(WFHARD);
            auto s1 = ldelnewline();
            auto s2 = TRUE;
            if (FALSE != kflag) { s2 = kinsert(10); } // \n;
            if (FALSE == s1 || FALSE == s2) { return FALSE; }
            n--;
            continue;
        }
        lchange(WFHARD);
        if (FALSE != kflag) {
            for (auto i=0; i <chunk; ++i) {
                if (FALSE == kinsert((int)dotp->l_text[doto + i] & 0xFF)) { return FALSE; }
            }
        }
        memmove(dotp->l_text + doto, dotp->l_text + doto + chunk, (size_t)(dotp->l_used - doto - chunk));
        dotp->l_used -= chunk;
        auto wp_l = curwp;
        if (dotp == wp_l->w_dotp && wp_l->w_doto >= doto) {
            wp_l->w_doto -= chunk;
            if (doto > wp_l->w_doto) {
                wp_l->w_doto = doto;
            }
        }
        if (dotp == wp_l->w_markp && wp_l->w_marko >= doto) {
            wp_l->w_marko -= chunk;
            if (doto > wp_l->w_marko) {
                wp_l->w_marko = doto;
            }
        }
        n -= (long)chunk;
    }
    return TRUE;
}

 string getctext() {
    auto lp = curwp->w_dotp;
    auto size = lp->l_used;
    static char rline[1024];
    if (size >= 1024) {
        size = 1023;
    }
    memcpy(rline, lp->l_text, (size_t)size);
    rline[size] = (char)0;
    return (string)rline;
}

 int putctext(string iline) {
    curwp->w_doto = 0;
    if (TRUE != killtext(TRUE, 1)) { return FALSE; }
    if (TRUE != linstr(iline)) { return FALSE; }
    if (FALSE == lnewline()) { return FALSE; }
    backline(TRUE, 1);
    return TRUE;
}

 int ldelnewline() {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    auto lp1 = curwp->w_dotp;
    auto lp2 = lp1->l_fp;
    if (curbp->b_linep == lp2) {
        if (0 == lp1->l_used) {
            lfree(lp1);
        }
        return TRUE;
    }
    if (lp2->l_used <= lp1->l_size - lp1->l_used) {
        memcpy(lp1->l_text + lp1->l_used, lp2->l_text, (size_t)lp2->l_used);
        auto wp_l = curwp;
        if (lp2 == wp_l->w_linep) {
            wp_l->w_linep = lp1;
        }
        if (lp2 == wp_l->w_dotp) {
            wp_l->w_dotp = lp1;
            wp_l->w_doto += lp1->l_used;
        }
        if (lp2 == wp_l->w_markp) {
            wp_l->w_markp = lp1;
            wp_l->w_marko += lp1->l_used;
        }
        lp1->l_used += lp2->l_used;
        lp1->l_fp = lp2->l_fp;
        lp2->l_fp->l_bp = lp1;
        free(lp2);
        return TRUE;
    }
    auto lp3 = lalloc(lp1->l_used + lp2->l_used);
    if (NULL == lp3) { return FALSE; }
    memcpy(lp3->l_text, lp1->l_text, (size_t)lp1->l_used);
    memcpy(lp3->l_text + lp1->l_used, lp2->l_text, (size_t)lp2->l_used);
    lp1->l_bp->l_fp = lp3;
    lp3->l_fp = lp2->l_fp;
    lp2->l_fp->l_bp = lp3;
    lp3->l_bp = lp1->l_bp;
    auto wp_l = curwp;
    if (lp1 == wp_l->w_linep || lp2 == wp_l->w_linep) {
        wp_l->w_linep = lp3;
    }
    if (lp1 == wp_l->w_dotp) {
        wp_l->w_dotp = lp3;
    } else if (lp2 == wp_l->w_dotp) {
        wp_l->w_dotp = lp3;
        wp_l->w_doto += lp1->l_used;
    }
    if (lp1 == wp_l->w_markp) {
        wp_l->w_markp = lp3;
    } else if (lp2 == wp_l->w_markp) {
        wp_l->w_markp = lp3;
        wp_l->w_marko += lp1->l_used;
    }
    free(lp1);
    free(lp2);
    return TRUE;
}

 void kdelete() {
    if (NULL != kbufh) {
        kbufp = kbufh;
        while (NULL != kbufp) {
            auto kp = kbufp->d_next;
            free(kbufp);
            kbufp = kp;
        }
        kbufh = NULL;
        kbufp = NULL;
    }
    kused = KBLOCK;
}

 int kinsert(int c) {
    if (kused >= KBLOCK) {
        auto nchunk = (weird(struct kill, 1))alloc(sizeof(struct kill));
        if (NULL == nchunk) { return FALSE; }
        if (NULL == kbufh) {
            kbufh = nchunk;
        }
        if (NULL != kbufp) {
            kbufp->d_next = nchunk;
        }
        kbufp = nchunk;
        kbufp->d_next = NULL;
        kused = 0;
    }
    kbufp->d_chunk[kused] = (char)c;
    kused++;
    return TRUE;
}

 int yank(int f, int n) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return FALSE; }
    if (NULL == kbufh) { return TRUE; }

    while (n > 0) {
        auto kp = kbufh;
        while (NULL != kp) {
            auto limit = 0;
            if (NULL == kp->d_next) {
                limit = kused;
            } else {
                limit = KBLOCK;
            }
            for (auto i=0; i <limit; ++i) {
                auto c = (int)kp->d_chunk[i] & 0xFF;
                if (13 == c) { // \r
                    if (FALSE == lnewline()) { return FALSE; }
                    auto next_c = (int)kp->d_chunk[i+1] & 0xFF;
                    if (10 == next_c && i+1 < limit) {
                        i++;
                    }
                } else if (10 == c) { // \n
                    if (FALSE == lnewline()) { return FALSE; }
                } else {
                    if (FALSE == linsert_byte(1, c)) { return FALSE; }
                }
            }
            kp = kp->d_next;
        }
        n--;
    }
    return TRUE;
}

 int linsert_block(string block, int len) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    auto start = 0;
    for (auto i=0; i <=len; ++i) {
        auto is_end = (i == len);
        auto is_nl = FALSE;
        if (FALSE == is_end) {
            if (10 == (int)block[i] || 13 == (int)block[i]) { is_nl = TRUE; };
        }
        if (TRUE == is_end || TRUE == is_nl) {
            auto segment_len = i - start;
            if (segment_len > 0) {
                auto lp1 = curwp->w_dotp;
                auto doto = curwp->w_doto;
                if (curbp->b_linep == lp1) {
                    auto lp2 = lalloc(segment_len);
                    if (NULL == lp2) { return FALSE; }
                    auto lp3 = lp1->l_bp;
                    lp3->l_fp = lp2;
                    lp2->l_fp = lp1;
                    lp1->l_bp = lp2;
                    lp2->l_bp = lp3;
                    memcpy(lp2->l_text, block + (size_t)start, (size_t)segment_len);
                    curwp->w_dotp = lp2;
                    curwp->w_doto = segment_len;
                } else {
                    auto lp2 = (weird(struct line, 1))NULL;
                    if (lp1->l_used + segment_len > lp1->l_size) {
                        lp2 = lalloc(lp1->l_used + segment_len);
                        if (NULL == lp2) { return FALSE; }
                        memcpy(lp2->l_text, lp1->l_text, (size_t)doto);
                        memcpy(lp2->l_text + doto + segment_len, lp1->l_text + doto, (size_t)(lp1->l_used - doto));
                        lp1->l_bp->l_fp = lp2;
                        lp2->l_fp = lp1->l_fp;
                        lp1->l_fp->l_bp = lp2;
                        lp2->l_bp = lp1->l_bp;
                        free(lp1);
                        curwp->w_dotp = lp2;
                    } else {
                        lp2 = lp1;
                        memmove(lp2->l_text + doto + segment_len, lp2->l_text + doto, (size_t)(lp2->l_used - doto));
                        lp2->l_used += segment_len;
                    }
                    memcpy(lp2->l_text + doto, block + (size_t)start, (size_t)segment_len);
                    curwp->w_doto += segment_len;
                    auto wp_l = curwp;
                    if (lp1 == wp_l->w_linep) { wp_l->w_linep = lp2; };
                    if (lp1 == wp_l->w_dotp) { wp_l->w_dotp = lp2; };
                    if (lp1 == wp_l->w_markp) { wp_l->w_markp = lp2; };
                }
            }
            if (i < len) {
                if (FALSE == lnewline()) { return FALSE; }
                if (13 == (int)block[i] && 10 == (int)block[i+1] && i+1 < len) {
                    i++;
                }
                start = i + 1;
            }
        }
    }
    lchange(WFHARD);
    return TRUE;
}

// ---- /home/yjlee/namo/buffer.margo ----
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 int usebuffer(int f, int n) {
    char bufn[16]; // NBUFN
    auto s = minibuf_input("Use buffer: ", (weird(char, 1))bufn, 16);
    if (s != TRUE) { return s; }
    auto bp = bfind((string)bufn, TRUE, 0);
    if (bp == NULL) { return FALSE; }
    return swbuffer(bp);
}

 int nextbuffer(int f, int n) {
    auto bp = (weird(struct buffer, 1))NULL;
    auto bbp = (weird(struct buffer, 1))NULL;
    if (f == FALSE) { n = 1; };
    if (n < 1) { return FALSE; }
    bbp = curbp;
    while (n > 0) {
        bp = bbp->b_bufp;
        while (bp == NULL || 0 != (bp->b_flag & BFINVS)) {
            if (bp == NULL) {
                bp = bheadp;
            } else {
                bp = bp->b_bufp;
            }
            if (bp == bbp) { return FALSE; }
        }
        bbp = bp;
        n--;
    }
    return swbuffer(bp);
}

 int swbuffer(weird(struct buffer, 1) bp) {
    if (0 == --curbp->b_nwnd) {
        curbp->b_dotp = curwp->w_dotp;
        curbp->b_doto = curwp->w_doto;
        curbp->b_markp = curwp->w_markp;
        curbp->b_marko = curwp->w_marko;
    }
    curbp = bp;
    if (TRUE != curbp->b_active) {
        readin((string)curbp->b_fname, TRUE);
        curbp->b_dotp = lforw(curbp->b_linep);
        curbp->b_doto = 0;
        curbp->b_active = TRUE;
        curbp->b_mode |= gmode;
    }
    curwp->w_bufp = bp;
    curwp->w_linep = bp->b_linep;
    curwp->w_flag = (char)((int)curwp->w_flag | (WFMODE | WFFORCE | WFHARD));
    if (0 == bp->b_nwnd++) {
        curwp->w_dotp = bp->b_dotp;
        curwp->w_doto = bp->b_doto;
        curwp->w_markp = bp->b_markp;
        curwp->w_marko = bp->b_marko;
    }
    return TRUE;
}

 int killbuffer(int f, int n) {
    char bufn[16]; // NBUFN
    auto s = minibuf_input("Kill buffer: ", (weird(char, 1))bufn, 16);
    if (s != TRUE) { return s; }
    auto bp = bfind((string)bufn, FALSE, 0);
    if (bp == NULL) { return TRUE; }
    if (0 != (bp->b_flag & BFINVS)) { return TRUE; }
    return zotbuf(bp);
}

 int zotbuf(weird(struct buffer, 1) bp) {
    char closed_fname[2048]; // NFILEN
    if (bp->b_nwnd != 0) {
        mlwrite("Buffer is being displayed");
        return FALSE;
    }
    mystrscpy(closed_fname, (string)bp->b_fname, 2048);
    auto s = bclear(bp);
    if (s != TRUE) { return s; }
    free(bp->b_linep);
    auto bp1 = (weird(struct buffer, 1))NULL;
    auto bp2 = bheadp;
    while (bp2 != bp) {
        bp1 = bp2;
        bp2 = bp2->b_bufp;
    }
    if (bp1 == NULL) {
        bheadp = bp2->b_bufp;
    } else {
        bp1->b_bufp = bp2->b_bufp;
    }
    free(bp);
    namo_handle_closed_file((string)closed_fname);
    return TRUE;
}

 void bfreeall() {
    auto bp = bheadp;
    while (bp != NULL) {
        auto next = bp->b_bufp;
        auto hlp = bp->b_linep;
        if (hlp != NULL) {
            auto lp = hlp->l_fp;
            while (lp != hlp) {
                auto nlp = lp->l_fp;
                free(lp);
                lp = nlp;
            }
            free(hlp);
        }
        free(bp);
        bp = next;
    }
    bheadp = NULL;
}

 int namebuffer(int f, int n) {
    char bufn[16]; // NBUFN
    while (true) {
        if (minibuf_input("Change buffer name to: ", (weird(char, 1))bufn, 16) != TRUE) { return FALSE; }
        auto bp = bheadp;
        auto found = FALSE;
        while (bp != NULL) {
            if (bp != curbp) {
                if (0 == strcmp((string)bufn, bp->b_bname)) { found = TRUE; break; };
            }
            bp = bp->b_bufp;
        }
        if (FALSE == found) { break; }
    }
    mystrscpy(curbp->b_bname, (string)bufn, 16);
    curwp->w_flag = (char)((int)curwp->w_flag | WFMODE);
    mlerase();
    return TRUE;
}

 void ltoa(weird(char, 1) buf, int width, long num) {
    buf[width] = (char)0;
    while (num >= 10L) {
        buf[--width] = (char)((int)(num % 10L) + 48); // '0'
        num /= 10L;
    }
    buf[--width] = (char)((int)num + 48); // '0'
    while (width != 0) {
        buf[--width] = (char)32; // ' '
    }
}

 int addline(string text) {
    auto ntext = (int)strlen(text);
    auto lp = lalloc(ntext);
    if (lp == NULL) { return FALSE; }
    for (auto i=0; i <ntext; ++i) {
        lputc(lp, i, (int)text[i]);
    }
    blistp->b_linep->l_bp->l_fp = lp;
    lp->l_bp = blistp->b_linep->l_bp;
    blistp->b_linep->l_bp = lp;
    lp->l_fp = blistp->b_linep;
    if (blistp->b_dotp == blistp->b_linep) { blistp->b_dotp = lp; };
    return TRUE;
}

 int anycb() {
    auto bp = bheadp;
    while (bp != NULL) {
        if (0 == (bp->b_flag & BFINVS) && 0 != (bp->b_flag & BFCHG)) { return TRUE; }
        bp = bp->b_bufp;
    }
    return FALSE;
}

 weird(struct buffer, 1) bfind(string bname, int cflag, int bflag) {
    auto bp = bheadp;
    while (bp != NULL) {
        if (0 == strcmp(bname, bp->b_bname)) { return bp; }
        bp = bp->b_bufp;
    }
    if (cflag != FALSE) {
        bp = (weird(struct buffer, 1))alloc(sizeof(buffer));
        if (bp == NULL) { return NULL; }
        auto lp = lalloc(0);
        if (lp == NULL) {
            free(bp);
            return NULL;
        }
        if (bheadp == NULL || strcmp(bheadp->b_bname, bname) > 0) {
            bp->b_bufp = bheadp;
            bheadp = bp;
        } else {
            auto sb = bheadp;
            while (sb->b_bufp != NULL) {
                if (strcmp(sb->b_bufp->b_bname, bname) > 0) { break; }
                sb = sb->b_bufp;
            }
            bp->b_bufp = sb->b_bufp;
            sb->b_bufp = bp;
        }
        bp->b_active = TRUE;
        bp->b_dotp = lp;
        bp->b_doto = 0;
        bp->b_markp = NULL;
        bp->b_marko = 0;
        bp->b_flag = (char)bflag;
        bp->b_mode = gmode;
        bp->b_nwnd = 0;
        bp->b_linep = lp;
        bp->b_tabsize = 7;
        strcpy(bp->b_fname, "");
        strcpy(bp->b_bname, bname);
        lp->l_fp = lp;
        lp->l_bp = lp;
    }
    return bp;
}

 void cleanup_backup(weird(struct buffer, 1) bp, int force) {
    if (0 != force || 0 != makebackup) {
        if (0 == (bp->b_flag & BFINVS) && 0 != (int)bp->b_fname[0]) {
            char backupName[2048];
            auto bfn_len = (int)strlen(bp->b_fname);
            if (bfn_len + 2 < 2048) {
                strcpy(backupName, bp->b_fname);
                strcat(backupName, "~");
                unlink((string)backupName);
            }
        }
    }
}

 int bclear(weird(struct buffer, 1) bp) {
    cleanup_backup(bp, FALSE);
    if (0 == (bp->b_flag & BFINVS) && 0 != (bp->b_flag & BFCHG)) {
        auto s = mlyesno("Discard changes");
        if (s != TRUE) { return s; }
    }
    bp->b_flag = (char)((int)bp->b_flag & (~BFCHG));
    while (lforw(bp->b_linep) != bp->b_linep) {
        lfree(lforw(bp->b_linep));
    }
    bp->b_dotp = bp->b_linep;
    bp->b_doto = 0;
    bp->b_markp = NULL;
    bp->b_marko = 0;
    return TRUE;
}

 int unmark(int f, int n) {
    curbp->b_flag = (char)((int)curbp->b_flag & (~BFCHG));
    curwp->w_flag = (char)((int)curwp->w_flag | WFMODE);
    return TRUE;
}

// ---- /home/yjlee/namo/window.margo ----
#include <stdio.h>

 int reposition(int f, int n) {
    if (f == FALSE) {
        n = 0;
    }
    curwp->w_force = (char)n;
    curwp->w_flag = (char)((int)curwp->w_flag | WFFORCE);
    return TRUE;
}

 int redraw(int f, int n) {
    if (f == FALSE) {
        sgarbf = TRUE;
    } else {
        curwp->w_force = 0;
        curwp->w_flag = (char)((int)curwp->w_flag | WFFORCE);
    }
    return TRUE;
}

 int newsize(int f, int n) {
    if (f == FALSE) {
        n = term->t_mrow + 1;
    }
    if (n < 3 || n > term->t_mrow + 1) {
        mlwrite("%%Screen size out of range");
        return FALSE;
    }
    if (term->t_nrow == (short)(n - 1)) { return TRUE; }
    curwp->w_flag = (char)((int)curwp->w_flag | (WFHARD | WFMODE));
    term->t_nrow = (short)(n - 1);
    sgarbf = TRUE;
    return TRUE;
}

 int newwidth(int f, int n) {
    if (f == FALSE) {
        n = term->t_mcol;
    }
    if (n < 10 || n > term->t_mcol) {
        mlwrite("%%Screen width out of range");
        return FALSE;
    }
    term->t_ncol = (short)n;
    term->t_margin = (short)(n / 10);
    term->t_scrsiz = (short)(n - (term->t_margin * 2));
    curwp->w_flag = (char)((int)curwp->w_flag | (WFHARD | WFMOVE | WFMODE));
    sgarbf = TRUE;
    return TRUE;
}

// ---- /home/yjlee/namo/basic.margo ----
#include <stdio.h>
#include <stdlib.h>

extern  int next_column(int old, unsigned int c, int tab_width);
extern  int minibuf_input(string prompt, weird(char, 1) buffer, int nbuf);
extern  int getccol(int bflg);
extern  int namo_text_rows();
extern  int inword();
extern  int forwline(int f, int n);
extern  int backline(int f, int n);
extern  int backpage(int f, int n);
extern  int forwpage(int f, int n);

 static int getgoal(weird(struct line, 1) dlp) {
    auto col = 0;
    auto dbo = 0;
    auto len = llength(dlp);
    while (dbo != len) {
        unsigned int c = 0;
        auto width = utf8_to_unicode((weird(unsigned char, 1))dlp->l_text, (unsigned int)dbo, (unsigned int)len, (weird(unsigned int, 1))(&c));
        col = next_column(col, c, tab_width);
        if (col > curgoal) { break; }
        dbo += (int)width;
    }
    return dbo;
}

 int gotobol(int f, int n) {
    curwp->w_doto = 0;
    return TRUE;
}

 int backchar(int f, int n) {
    if (n < 0) { return forwchar(f, -n); }
    while (n > 0) {
        if (curwp->w_doto == 0) {
            auto lp = lback(curwp->w_dotp);
            if (lp == curbp->b_linep) { return FALSE; }
            curwp->w_dotp = lp;
            curwp->w_doto = llength(lp);
            curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
        } else {
            while (curwp->w_doto > 0) {
                curwp->w_doto--;
                if (0 != is_beginning_utf8((unsigned char)lgetc(curwp->w_dotp, curwp->w_doto))) { break; }
            }
        }
        n--;
    }
    return TRUE;
}

 int gotoeol(int f, int n) {
    curwp->w_doto = llength(curwp->w_dotp);
    return TRUE;
}

 int forwchar(int f, int n) {
    if (n < 0) { return backchar(f, -n); }
    while (n > 0) {
        auto len = llength(curwp->w_dotp);
        if (curwp->w_doto == len) {
            if (curwp->w_dotp == curbp->b_linep) { return FALSE; }
            curwp->w_dotp = lforw(curwp->w_dotp);
            curwp->w_doto = 0;
            curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
        } else {
            unsigned int c = 0;
            auto bytes = utf8_to_unicode((weird(unsigned char, 1))curwp->w_dotp->l_text, (unsigned int)curwp->w_doto, (unsigned int)len, (weird(unsigned int, 1))(&c));
            curwp->w_doto += (int)bytes;
            if (curwp->w_doto > len) { curwp->w_doto = len; };
        }
        n--;
    }
    return TRUE;
}

 int gotoline(int f, int n) {
    if (f == FALSE) {
        char arg[1024];
        auto status = minibuf_input("Line to GOTO: ", (weird(char, 1))arg, 1024);
        if (status != TRUE) {
            mlwrite("(Aborted)");
            return status;
        }
        n = atoi((string)arg);
    }
    if (n == 0) { gotoeob(f, n); return TRUE; }
    if (n < 0) { return FALSE; }
    gotobob(f, n);
    return forwline(f, n - 1);
}

 void gotobob(int f, int n) {
    curwp->w_dotp = lforw(curbp->b_linep);
    curwp->w_doto = 0;
    curwp->w_flag = (char)((int)curwp->w_flag | WFHARD);
}

 void gotoeob(int f, int n) {
    curwp->w_dotp = curbp->b_linep;
    curwp->w_doto = 0;
    curwp->w_flag = (char)((int)curwp->w_flag | WFHARD);
}

 int forwline(int f, int n) {
    if (n < 0) { return backline(f, -n); }
    if (curwp->w_dotp == curbp->b_linep) { return FALSE; }
    if (0 == (lastflag & CFCPCN)) { curgoal = getccol(FALSE); };
    thisflag |= CFCPCN;
    auto dlp = curwp->w_dotp;
    while (n > 0 && dlp != curbp->b_linep) {
        dlp = lforw(dlp);
        n--;
    }
    curwp->w_dotp = dlp;
    curwp->w_doto = getgoal(dlp);
    curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
    return TRUE;
}

 int backline(int f, int n) {
    if (n < 0) { return forwline(f, -n); }
    if (lback(curwp->w_dotp) == curbp->b_linep) { return FALSE; }
    if (0 == (lastflag & CFCPCN)) { curgoal = getccol(FALSE); };
    thisflag |= CFCPCN;
    auto dlp = curwp->w_dotp;
    while (n > 0 && lback(dlp) != curbp->b_linep) {
        dlp = lback(dlp);
        n--;
    }
    curwp->w_dotp = dlp;
    curwp->w_doto = getgoal(dlp);
    curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
    return TRUE;
}

 static int is_new_para() {
    auto len = llength(curwp->w_dotp);
    for (auto i=0; i <len; ++i) {
        auto c = lgetc(curwp->w_dotp, i);
        if (c == (int)' ' || c == 0x09) { // TAB
            if (0 != justflag) { continue; }
            return 1;
        }
        if (FALSE == isletter(c)) { return 1; }
        return 0;
    }
    return 1;
}

 void gotobop(int f, int n) {
    if (n < 0) { gotoeop(f, -n); return; }
    while (n > 0) {
        auto suc = backchar(FALSE, 1);
        while (FALSE == inword() && TRUE == suc) { suc = backchar(FALSE, 1); };
        curwp->w_doto = 0;
        while (lback(curwp->w_dotp) != curbp->b_linep) {
            if (1 == is_new_para()) { break; }
            curwp->w_dotp = lback(curwp->w_dotp);
        }
        suc = forwchar(FALSE, 1);
        while (TRUE == suc && FALSE == inword()) { suc = forwchar(FALSE, 1); };
        n--;
    }
    curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
}

 void gotoeop(int f, int n) {
    if (n < 0) { gotobop(f, -n); return; }
    while (n > 0) {
        auto suc = forwchar(FALSE, 1);
        while (FALSE == inword() && TRUE == suc) { suc = forwchar(FALSE, 1); };
        curwp->w_doto = 0;
        if (TRUE == suc) { curwp->w_dotp = lforw(curwp->w_dotp); };
        while (curwp->w_dotp != curbp->b_linep) {
            if (1 == is_new_para()) { break; }
            curwp->w_dotp = lforw(curwp->w_dotp);
        }
        suc = backchar(FALSE, 1);
        while (TRUE == suc && FALSE == inword()) { suc = backchar(FALSE, 1); };
        curwp->w_doto = llength(curwp->w_dotp);
        n--;
    }
    curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
}

 int forwpage(int f, int n) {
    if (f == FALSE) {
        n = namo_text_rows() - 2;
        if (n <= 0) { n = 1; };
    } else if (n < 0) { return backpage(f, -n); }
    else { n *= namo_text_rows(); };
    auto lp = curwp->w_linep;
    while (n > 0 && lp != curbp->b_linep) {
        lp = lforw(lp);
        n--;
    }
    curwp->w_linep = lp;
    curwp->w_dotp = lp;
    curwp->w_doto = 0;
    curwp->w_flag = (char)((int)curwp->w_flag | WFHARD);
    return TRUE;
}

 int backpage(int f, int n) {
    if (f == FALSE) {
        n = namo_text_rows() - 2;
        if (n <= 0) { n = 1; };
    } else if (n < 0) { return forwpage(f, -n); }
    else { n *= namo_text_rows(); };
    auto lp = curwp->w_linep;
    while (n > 0 && lback(lp) != curbp->b_linep) {
        lp = lback(lp);
        n--;
    }
    curwp->w_linep = lp;
    curwp->w_dotp = lp;
    curwp->w_doto = 0;
    curwp->w_flag = (char)((int)curwp->w_flag | WFHARD);
    return TRUE;
}

 int setmark(int f, int n) {
    curwp->w_markp = curwp->w_dotp;
    curwp->w_marko = curwp->w_doto;
    mlwrite("(Mark set)");
    return TRUE;
}

 int swapmark(int f, int n) {
    if (curwp->w_markp == NULL) {
        mlwrite("No mark in this window");
        return FALSE;
    }
    auto odotp = curwp->w_dotp;
    auto odoto = curwp->w_doto;
    curwp->w_dotp = curwp->w_markp;
    curwp->w_doto = curwp->w_marko;
    curwp->w_markp = odotp;
    curwp->w_marko = odoto;
    curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
    return TRUE;
}

// ---- /home/yjlee/namo/input.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

 static int erase_prev_glyph(weird(char, 1) buf, weird(int, 1) cpos) {
    if (NULL == buf || NULL == cpos || cpos[0] <= 0) { return 0; }
    auto start = cpos[0];
    while (true) {
        start--;
        if (start <= 0 || 0 != is_beginning_utf8((unsigned char)buf[start])) { break; }
    }
    unsigned int uc = 0;
    utf8_to_unicode((weird(unsigned char, 1))buf, (unsigned int)start, (unsigned int)(cpos[0] - start), (weird(unsigned int, 1))(&uc));
    auto width = mystrnlen_raw_w(uc);
    cpos[0] = start;
    while (width > 0) {
        TTputc(8); // '\b'
        TTputc(32); // ' '
        TTputc(8); // '\b'
        if (ttcol > 0) { ttcol--; }
        width--;
    }
    return 1;
}

 int minibuf_input(string prompt, weird(char, 1) buffer, int nbuf) {
    auto cpos = 0;
    auto c = 0;
    mlforce(prompt);
    while (true) {
        c = getcmd();
        if (13 == c || 10 == c) { break; } // \r, \n
        if (7 == c) { // Ctrl+G
            return ABORT;
        }
        if (8 == c || 127 == c) { // \b, DEL
            erase_prev_glyph(buffer, (weird(int, 1))(&cpos));
            continue;
        }
        if (cpos < nbuf - 1) {
            buffer[cpos] = (char)c;
            cpos++;
            TTputc(c);
            ttcol++;
        }
    }
    buffer[cpos] = (char)0;
    return TRUE;
}

 int getcmd() {
    return TTgetc();
}

// ---- /home/yjlee/namo/platform.margo ----
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 void namo_get_user_data_dir(weird(char, 1) out, int cap) {
    auto home = getenv("HOME");
    if (home == NULL) {
        mystrscpy(out, "/tmp", cap);
        return;
    }
    snprintf(out, (unsigned int)cap, "%s/.local/share/namo", home);
}

 void namo_get_user_config_dir(weird(char, 1) out, int cap) {
    auto home = getenv("HOME");
    if (home == NULL) {
        mystrscpy(out, "/tmp", cap);
        return;
    }
    snprintf(out, (unsigned int)cap, "%s/.config/namo", home);
}

 void namo_path_join(weird(char, 1) out, int cap, string a, string b) {
    if (a == NULL || a[0] == 0) {
        mystrscpy(out, b, cap);
        return;
    }
    auto len = (int)strlen(a);
    if (a[len-1] == '/') {
        snprintf(out, (unsigned int)cap, "%s%s", a, b);
    } else {
        snprintf(out, (unsigned int)cap, "%s/%s", a, b);
    }
}

 bool namo_file_exists(string path) {
    auto f = namo_c_fopen(path, "r");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

 void namo_normalize_path(weird(char, 1) path) {
    // Basic normalization: convert backslashes to forward slashes
    auto i = 0;
    while (path[i] != 0) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
        i++;
    }
}

 string namo_getenv(string name) {
    return (string)getenv(name);
}

// ---- /home/yjlee/namo/colorscheme.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int HighlightStyleID;
const HighlightStyleID HL_NORMAL = 0;
const HighlightStyleID HL_COMMENT = 1;
const HighlightStyleID HL_STRING = 2;
const HighlightStyleID HL_NUMBER = 3;
const HighlightStyleID HL_BRACKET = 4;
const HighlightStyleID HL_OPERATOR = 5;
const HighlightStyleID HL_KEYWORD = 6;
const HighlightStyleID HL_TYPE = 7;
const HighlightStyleID HL_FUNCTION = 8;
const HighlightStyleID HL_FLOW = 9;
const HighlightStyleID HL_PREPROC = 10;
const HighlightStyleID HL_RETURN = 11;
const HighlightStyleID HL_ESCAPE = 12;
const HighlightStyleID HL_CONTROL = 13;
const HighlightStyleID HL_TERNARY = 14;
const HighlightStyleID HL_ERROR = 15;
const HighlightStyleID HL_NOTICE = 16;
const HighlightStyleID HL_SELECTION = 17;
const HighlightStyleID HL_HEADER = 18;
const HighlightStyleID HL_MD_BOLD = 19;
const HighlightStyleID HL_MD_ITALIC = 20;
const HighlightStyleID HL_MD_UNDERLINE = 21;
const HighlightStyleID HL_LINENUM = 22;
const HighlightStyleID HL_COUNT = 23;

struct HighlightStyle {
    int fg;
    int bg;
    bool bold;
    bool underline;
    bool italic;
};
typedef struct HighlightStyle HighlightStyle;

static char current_scheme_name[64] = "default";
static HighlightStyle styles[HL_COUNT];

 static void set_default_scheme() {
    styles[HL_NORMAL].fg = -1; styles[HL_NORMAL].bg = -1; styles[HL_NORMAL].bold = false; styles[HL_NORMAL].underline = false; styles[HL_NORMAL].italic = false;
    styles[HL_COMMENT].fg = 8; styles[HL_COMMENT].bg = -1; styles[HL_COMMENT].bold = false; styles[HL_COMMENT].underline = false; styles[HL_COMMENT].italic = false;
    styles[HL_STRING].fg = 2; styles[HL_STRING].bg = -1; styles[HL_STRING].bold = false; styles[HL_STRING].underline = false; styles[HL_STRING].italic = false;
    styles[HL_NUMBER].fg = 5; styles[HL_NUMBER].bg = -1; styles[HL_NUMBER].bold = false; styles[HL_NUMBER].underline = false; styles[HL_NUMBER].italic = false;
    styles[HL_BRACKET].fg = 6; styles[HL_BRACKET].bg = -1; styles[HL_BRACKET].bold = false; styles[HL_BRACKET].underline = false; styles[HL_BRACKET].italic = false;
    styles[HL_OPERATOR].fg = 6; styles[HL_OPERATOR].bg = -1; styles[HL_OPERATOR].bold = false; styles[HL_OPERATOR].underline = false; styles[HL_OPERATOR].italic = false;
    styles[HL_KEYWORD].fg = 3; styles[HL_KEYWORD].bg = -1; styles[HL_KEYWORD].bold = true; styles[HL_KEYWORD].underline = false; styles[HL_KEYWORD].italic = false;
    styles[HL_TYPE].fg = 6; styles[HL_TYPE].bg = -1; styles[HL_TYPE].bold = false; styles[HL_TYPE].underline = false; styles[HL_TYPE].italic = false;
    styles[HL_FUNCTION].fg = 4; styles[HL_FUNCTION].bg = -1; styles[HL_FUNCTION].bold = false; styles[HL_FUNCTION].underline = false; styles[HL_FUNCTION].italic = false;
    styles[HL_FLOW].fg = 3; styles[HL_FLOW].bg = -1; styles[HL_FLOW].bold = true; styles[HL_FLOW].underline = false; styles[HL_FLOW].italic = false;
    styles[HL_PREPROC].fg = 1; styles[HL_PREPROC].bg = -1; styles[HL_PREPROC].bold = false; styles[HL_PREPROC].underline = false; styles[HL_PREPROC].italic = false;
    styles[HL_RETURN].fg = 9; styles[HL_RETURN].bg = -1; styles[HL_RETURN].bold = true; styles[HL_RETURN].underline = false; styles[HL_RETURN].italic = false;
    styles[HL_ESCAPE].fg = 14; styles[HL_ESCAPE].bg = -1; styles[HL_ESCAPE].bold = false; styles[HL_ESCAPE].underline = false; styles[HL_ESCAPE].italic = false;
    styles[HL_CONTROL].fg = 9; styles[HL_CONTROL].bg = -1; styles[HL_CONTROL].bold = false; styles[HL_CONTROL].underline = true; styles[HL_CONTROL].italic = false;
    styles[HL_TERNARY].fg = 3; styles[HL_TERNARY].bg = -1; styles[HL_TERNARY].bold = true; styles[HL_TERNARY].underline = false; styles[HL_TERNARY].italic = false;
    styles[HL_ERROR].fg = 9; styles[HL_ERROR].bg = -1; styles[HL_ERROR].bold = false; styles[HL_ERROR].underline = false; styles[HL_ERROR].italic = false;
    styles[HL_NOTICE].fg = 208; styles[HL_NOTICE].bg = -1; styles[HL_NOTICE].bold = true; styles[HL_NOTICE].underline = false; styles[HL_NOTICE].italic = false;
    styles[HL_SELECTION].fg = 0; styles[HL_SELECTION].bg = 11; styles[HL_SELECTION].bold = false; styles[HL_SELECTION].underline = false; styles[HL_SELECTION].italic = false;
    styles[HL_HEADER].fg = 4; styles[HL_HEADER].bg = -1; styles[HL_HEADER].bold = true; styles[HL_HEADER].underline = false; styles[HL_HEADER].italic = false;
    styles[HL_MD_BOLD].fg = -1; styles[HL_MD_BOLD].bg = -1; styles[HL_MD_BOLD].bold = true; styles[HL_MD_BOLD].underline = false; styles[HL_MD_BOLD].italic = false;
    styles[HL_MD_ITALIC].fg = 6; styles[HL_MD_ITALIC].bg = -1; styles[HL_MD_ITALIC].bold = false; styles[HL_MD_ITALIC].underline = false; styles[HL_MD_ITALIC].italic = true;
    styles[HL_MD_UNDERLINE].fg = -1; styles[HL_MD_UNDERLINE].bg = -1; styles[HL_MD_UNDERLINE].bold = false; styles[HL_MD_UNDERLINE].underline = true; styles[HL_MD_UNDERLINE].italic = false;
    styles[HL_LINENUM].fg = 8; styles[HL_LINENUM].bg = -1; styles[HL_LINENUM].bold = false; styles[HL_LINENUM].underline = false; styles[HL_LINENUM].italic = false;
}

 static int parse_color(string val) {
    if (0 == strcmp(val, "default")) { return -1; }
    if (35 == (int)val[0]) { // '#'
        int r, g, b;
        if (3 == sscanf(val + 1, "%02x%02x%02x", &r, &g, &b)) {
            return 0x01000000 | (r << 16) | (g << 8) | b;
        }
    }
    auto offset = 0;
    auto p = val;
    if (0 == strncmp(p, "bright_", 7)) {
        offset = 8;
        p = p + 7;
    }
    if (0 == strcmp(p, "black")) { return 0 + offset; }
    if (0 == strcmp(p, "red")) { return 1 + offset; }
    if (0 == strcmp(p, "green")) { return 2 + offset; }
    if (0 == strcmp(p, "yellow")) { return 3 + offset; }
    if (0 == strcmp(p, "blue")) { return 4 + offset; }
    if (0 == strcmp(p, "magenta")) { return 5 + offset; }
    if (0 == strcmp(p, "cyan")) { return 6 + offset; }
    if (0 == strcmp(p, "white")) { return 7 + offset; }
    return -1;
}

 static void parse_attributes(string value, weird(struct HighlightStyle, 1) style) {
    auto token = strtok(value, " ");
    while (NULL != token) {
        if (0 == strncmp(token, "fg=", 3)) {
            style->fg = parse_color(token + 3);
        } else if (0 == strncmp(token, "bg=", 3)) {
            style->bg = parse_color(token + 3);
        } else if (0 == strcmp(token, "bold=true")) {
            style->bold = true;
        } else if (0 == strcmp(token, "underline=true")) {
            style->underline = true;
        } else if (0 == strcmp(token, "italic=true")) {
            style->italic = true;
        }
        token = strtok(NULL, " ");
    }
}

 static bool is_safe_name(string name) {
    if (NULL == name) { return false; }
    auto len = (int)strlen(name);
    for (auto i=0; i <len; ++i) {
        auto c = (int)name[i];
        if (FALSE == isletter(c) && 46 != c && 95 != c && 45 != c) { return false; } // '.', '_', '-'
    }
    return true;
}

 static void load_scheme_file(string path) {
    auto f = fopen(path, "r");
    if (NULL == f) { return; }

    char line[256];
    bool in_styles = false;

    while (NULL != fgets(line, 256, f)) {
        auto p = line;
        while (0 != (int)p[0] && 0 != isspace((int)p[0] & 0xFF)) {
            p = p + 1;
        }

        if (0 == (int)p[0] || 59 == (int)p[0] || 35 == (int)p[0]) { continue; } // ';', '#'

        auto len = (int)strlen(p);
        while (len > 0 && 0 != isspace((int)p[len-1] & 0xFF)) {
            p[len-1] = (char)0;
            len--;
        }

        if (91 == (int)p[0]) { // '['
            if (0 == strcmp(p, "[styles]")) {
                in_styles = true;
            } else {
                in_styles = false;
            }
            continue;
        }

        if (FALSE == in_styles) { continue; }

        auto eq = strchr(p, 61); // '='
        if (NULL == eq) { continue; }
        eq[0] = (char)0;
        auto key = p;
        auto val = eq + 1;

        while (0 != (int)val[0] && 0 != isspace((int)val[0] & 0xFF)) {
            val = val + 1;
        }

        auto key_len = (int)strlen(key);
        while (key_len > 0 && 0 != isspace((int)key[key_len-1] & 0xFF)) {
            key[key_len-1] = (char)0;
            key_len--;
        }

        auto id = HL_COUNT;
        if (0 == strcmp(key, "normal")) { id = HL_NORMAL; };
        if (0 == strcmp(key, "comment")) { id = HL_COMMENT; };
        if (0 == strcmp(key, "string")) { id = HL_STRING; };
        if (0 == strcmp(key, "number")) { id = HL_NUMBER; };
        if (0 == strcmp(key, "bracket")) { id = HL_BRACKET; };
        if (0 == strcmp(key, "operator")) { id = HL_OPERATOR; };
        if (0 == strcmp(key, "keyword")) { id = HL_KEYWORD; };
        if (0 == strcmp(key, "type")) { id = HL_TYPE; };
        if (0 == strcmp(key, "function")) { id = HL_FUNCTION; };
        if (0 == strcmp(key, "flow")) { id = HL_FLOW; };
        if (0 == strcmp(key, "preproc")) { id = HL_PREPROC; };
        if (0 == strcmp(key, "return")) { id = HL_RETURN; };
        if (0 == strcmp(key, "escape")) { id = HL_ESCAPE; };
        if (0 == strcmp(key, "control")) { id = HL_CONTROL; };
        if (0 == strcmp(key, "ternary")) { id = HL_TERNARY; };
        if (0 == strcmp(key, "error")) { id = HL_ERROR; };
        if (0 == strcmp(key, "notice")) { id = HL_NOTICE; };
        if (0 == strcmp(key, "selection")) { id = HL_SELECTION; };
        if (0 == strcmp(key, "header")) { id = HL_HEADER; };
        if (0 == strcmp(key, "md_bold")) { id = HL_MD_BOLD; };
        if (0 == strcmp(key, "md_italic")) { id = HL_MD_ITALIC; };
        if (0 == strcmp(key, "md_underline")) { id = HL_MD_UNDERLINE; };
        if (0 == strcmp(key, "linenum")) { id = HL_LINENUM; };

        if (HL_COUNT != id) {
            parse_attributes((string)val, (weird(struct HighlightStyle, 1))(&styles[id]));
        }
    }
    fclose(f);

    if (-1 != styles[HL_NORMAL].bg) {
        for (auto i=0; i <(int)HL_COUNT; ++i) {
            if (i != (int)HL_NORMAL && i != (int)HL_SELECTION) {
                if (styles[i].bg == -1) {
                    styles[i].bg = styles[HL_NORMAL].bg;
                }
            }
        }
    }
}

 void colorscheme_init(string requested_name) {
    set_default_scheme();
    char name[64];
    mystrscpy(name, "default", 64);
    auto env_scheme = namo_getenv("NAMO_COLORSCHEME");
    if (NULL != env_scheme && 0 != (int)env_scheme[0] && TRUE == is_safe_name(env_scheme)) {
        mystrscpy(name, env_scheme, 64);
    } else {
        if (NULL != requested_name && 0 != (int)requested_name[0] && TRUE == is_safe_name(requested_name)) {
            mystrscpy(name, requested_name, 64);
        }
    }

    if (0 == strcmp((string)name, "default")) { return; }

    char dir[512];
    char path[1024];
    char file_name[76]; // 64 + 12

    namo_get_user_config_dir(dir, 512);
    namo_path_join(dir, 512, (string)dir, "colorscheme");

    snprintf(file_name, 76, "%s.namocolor", (string)name);
    namo_path_join(path, 1024, (string)dir, (string)file_name);
    if (TRUE == namo_file_exists((string)path)) {
        load_scheme_file((string)path);
        mystrscpy(current_scheme_name, (string)name, 64);
        return;
    }

    snprintf(file_name, 76, "%s.ini", (string)name);
    namo_path_join(path, 1024, (string)dir, (string)file_name);
    if (TRUE == namo_file_exists((string)path)) {
        load_scheme_file((string)path);
        mystrscpy(current_scheme_name, (string)name, 64);
        return;
    }

    namo_get_user_data_dir(dir, 512);
    namo_path_join(dir, 512, (string)dir, "colorscheme");

    snprintf(file_name, 76, "%s.namocolor", (string)name);
    namo_path_join(path, 1024, (string)dir, (string)file_name);
    if (TRUE == namo_file_exists((string)path)) {
        load_scheme_file((string)path);
        mystrscpy(current_scheme_name, (string)name, 64);
        return;
    }

    snprintf(file_name, 76, "%s.ini", (string)name);
    namo_path_join(path, 1024, (string)dir, (string)file_name);
    if (TRUE == namo_file_exists((string)path)) {
        load_scheme_file((string)path);
        mystrscpy(current_scheme_name, (string)name, 64);
        return;
    }
}

 HighlightStyle colorscheme_get(HighlightStyleID id) {
    if (0 > id || id >= HL_COUNT) { return styles[HL_NORMAL]; }
    return styles[id];
}

 string colorscheme_get_name() {
    return (string)current_scheme_name;
}

// ---- /home/yjlee/namo/region.margo ----
#include <stdio.h>
#include <string.h>

extern  int kinsert(int c);
extern  void lchange(int flag);

 static int namo_resolve_region(weird(struct region, 1) rp) {
    if (NULL == curwp->w_markp) {
        mlwrite("No mark set in this window");
        return FALSE;
    }
    return getregion(rp);
}

 static void namo_prime_kill_buffer() {
    if (0 == (lastflag & CFKILL)) { kdelete(); }
    thisflag |= CFKILL;
}

 static int namo_copy_region_to_kill(weird(struct region, 1) rp) {
    auto linep = rp->r_linep;
    auto loffs = rp->r_offset;
    auto remaining = rp->r_size;
    while (remaining > 0) {
        if (linep == curbp->b_linep) { return FALSE; }
        if (loffs == llength(linep)) {
            if (FALSE == kinsert((int)'\n')) { return FALSE; }
            linep = lforw(linep);
            loffs = 0;
        } else {
            if (FALSE == kinsert(lgetc(linep, loffs))) { return FALSE; }
            loffs++;
        }
        remaining--;
    }
    return TRUE;
}

 int kill_region_namo(int f, int n) {
    region r;
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (FALSE == namo_resolve_region((weird(struct region, 1))(&r))) { return FALSE; }
    namo_prime_kill_buffer();
    if (FALSE == namo_copy_region_to_kill((weird(struct region, 1))(&r))) { return FALSE; }
    curwp->w_dotp = r.r_linep;
    curwp->w_doto = r.r_offset;
    if (FALSE == ldelete(r.r_size, FALSE)) { return FALSE; }
    curwp->w_flag = (char)((int)curwp->w_flag | WFHARD);
    return TRUE;
}

 int killregion(int f, int n) {
    return kill_region_namo(f, n);
}

 int copy_region_namo(int f, int n) {
    region r;
    if (FALSE == namo_resolve_region((weird(struct region, 1))(&r))) { return FALSE; }
    namo_prime_kill_buffer();
    if (FALSE == namo_copy_region_to_kill((weird(struct region, 1))(&r))) { return FALSE; }
    mlwrite("(region copied)");
    return TRUE;
}

 int copyregion(int f, int n) {
    return copy_region_namo(f, n);
}

 int lowerregion(int f, int n) {
    region r;
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (FALSE == getregion((weird(struct region, 1))(&r))) { return FALSE; }
    lchange(WFHARD);
    auto linep = r.r_linep;
    auto loffs = r.r_offset;
    auto rem = r.r_size;
    while (rem > 0) {
        if (loffs == llength(linep)) {
            linep = lforw(linep);
            loffs = 0;
        } else {
            auto c = lgetc(linep, loffs);
            if (c >= (int)'A' && c <= (int)'Z') {
                lputc(linep, loffs, c + 32); // 'a' - 'A'
            }
            loffs++;
        }
        rem--;
    }
    return TRUE;
}

 int upperregion(int f, int n) {
    region r;
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (FALSE == getregion((weird(struct region, 1))(&r))) { return FALSE; }
    lchange(WFHARD);
    auto linep = r.r_linep;
    auto loffs = r.r_offset;
    auto rem = r.r_size;
    while (rem > 0) {
        if (loffs == llength(linep)) {
            linep = lforw(linep);
            loffs = 0;
        } else {
            auto c = lgetc(linep, loffs);
            if (c >= (int)'a' && c <= (int)'z') {
                lputc(linep, loffs, c - 32); // 'A' - 'a'
            }
            loffs++;
        }
        rem--;
    }
    return TRUE;
}

 int getregion(weird(struct region, 1) rp) {
    if (NULL == curwp->w_markp) {
        mlwrite("No mark set in this window");
        return FALSE;
    }
    if (curwp->w_dotp == curwp->w_markp) {
        rp->r_linep = curwp->w_dotp;
        if (curwp->w_doto < curwp->w_marko) {
            rp->r_offset = curwp->w_doto;
            rp->r_size = (long)(curwp->w_marko - curwp->w_doto);
        } else {
            rp->r_offset = curwp->w_marko;
            rp->r_size = (long)(curwp->w_doto - curwp->w_marko);
        }
        return TRUE;
    }
    auto blp = curwp->w_dotp;
    auto bsize = (long)curwp->w_doto;
    auto flp = curwp->w_dotp;
    auto fsize = (long)(llength(flp) - curwp->w_doto + 1);
    while (flp != curbp->b_linep || lback(blp) != curbp->b_linep) {
        if (flp != curbp->b_linep) {
            flp = lforw(flp);
            if (flp == curwp->w_markp) {
                rp->r_linep = curwp->w_dotp;
                rp->r_offset = curwp->w_doto;
                rp->r_size = fsize + (long)curwp->w_marko;
                return TRUE;
            }
            fsize += (long)(llength(flp) + 1);
        }
        if (lback(blp) != curbp->b_linep) {
            blp = lback(blp);
            bsize += (long)(llength(blp) + 1);
            if (blp == curwp->w_markp) {
                rp->r_linep = blp;
                rp->r_offset = curwp->w_marko;
                rp->r_size = bsize - (long)curwp->w_marko;
                return TRUE;
            }
        }
    }
    mlwrite("Bug: lost mark");
    return FALSE;
}

// ---- /home/yjlee/namo/word.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wctype.h>

extern  int forwdel(int f, int n);
extern  int backdel(int f, int n);
extern  int forwword(int f, int n);
extern  int gotoeol(int f, int n);

 int wrapword(int f, int n) {
    if (FALSE == backchar(0, 1)) { return FALSE; }
    auto cnt = 0;
    while (true) {
        auto c = lgetc(curwp->w_dotp, curwp->w_doto);
        if (c == 32 || c == 0x09) { break; } // 32 is ' '
        cnt++;
        if (FALSE == backchar(0, 1)) { return FALSE; }
        if (0 == curwp->w_doto) {
            gotoeol(FALSE, 0);
            return lnewline();
        }
    }
    if (FALSE == forwdel(0, 1)) { return FALSE; }
    if (FALSE == lnewline()) { return FALSE; }
    while (cnt > 0) {
        if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        cnt--;
    }
    return TRUE;
}

 int backword(int f, int n) {
    if (n < 0) { return forwword(f, -n); }
    if (FALSE == backchar(FALSE, 1)) { return FALSE; }
    while (n > 0) {
        while (FALSE == inword()) {
            if (FALSE == backchar(FALSE, 1)) { return FALSE; }
        }
        while (FALSE != inword()) {
            if (FALSE == backchar(FALSE, 1)) { return FALSE; }
        }
        n--;
    }
    return forwchar(FALSE, 1);
}

 int forwword(int f, int n) {
    if (n < 0) { return backword(f, -n); }
    while (n > 0) {
        while (TRUE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
        while (FALSE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
        n--;
    }
    return TRUE;
}

 int upperword(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return FALSE; }
    while (n > 0) {
        while (FALSE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
        while (TRUE == inword()) {
            unsigned int c = 0;
            auto bytes = lgetchar((weird(unsigned int, 1))(&c));
            auto nc = (unsigned int)towupper((wint_t)c);
            if (nc != c) {
                ldelete((long)bytes, FALSE);
                linsert(1, (int)nc);
            } else {
                forwchar(FALSE, 1);
            }
        }
        n--;
    }
    return TRUE;
}

 int lowerword(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return FALSE; }
    while (n > 0) {
        while (FALSE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
        while (TRUE == inword()) {
            unsigned int c = 0;
            auto bytes = lgetchar((weird(unsigned int, 1))(&c));
            auto nc = (unsigned int)towlower((wint_t)c);
            if (nc != c) {
                ldelete((long)bytes, FALSE);
                linsert(1, (int)nc);
            } else {
                forwchar(FALSE, 1);
            }
        }
        n--;
    }
    return TRUE;
}

 int capword(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return FALSE; }
    while (n > 0) {
        while (FALSE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
        if (TRUE == inword()) {
            unsigned int c = 0;
            auto bytes = lgetchar((weird(unsigned int, 1))(&c));
            auto nc = (unsigned int)towupper((wint_t)c);
            if (nc != c) {
                ldelete((long)bytes, FALSE);
                linsert(1, (int)nc);
            } else {
                forwchar(FALSE, 1);
            }
            while (TRUE == inword()) {
                bytes = lgetchar((weird(unsigned int, 1))(&c));
                nc = (unsigned int)towlower((wint_t)c);
                if (nc != c) {
                    ldelete((long)bytes, FALSE);
                    linsert(1, (int)nc);
                } else {
                    forwchar(FALSE, 1);
                }
            }
        }
        n--;
    }
    return TRUE;
}

 int delfword(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return FALSE; }
    if (0 == (lastflag & CFKILL)) { kdelete(); }
    thisflag |= CFKILL;
    auto dotp = curwp->w_dotp;
    auto doto = curwp->w_doto;
    while (FALSE == inword()) {
        if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
    }
    if (n == 0) {
        while (TRUE == inword()) {
            if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
        }
    } else {
        while (n > 0) {
            while (curwp->w_doto == llength(curwp->w_dotp)) {
                if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
            }
            while (TRUE == inword()) {
                if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
            }
            n--;
            if (0 != n) {
                while (FALSE == inword()) {
                    if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
                }
            }
        }
        while (curwp->w_doto == llength(curwp->w_dotp) || lgetc(curwp->w_dotp, curwp->w_doto) == 32 || lgetc(curwp->w_dotp, curwp->w_doto) == 0x09) {
            if (FALSE == forwchar(FALSE, 1)) { break; }
        }
    }
    auto lp = dotp;
    auto size = 0L;
    while (lp != curwp->w_dotp) {
        size += (long)(llength(lp) + 1);
        lp = lforw(lp);
    }
    size += (long)curwp->w_doto;
    size -= (long)doto;
    curwp->w_dotp = dotp;
    curwp->w_doto = doto;
    return ldelete(size, TRUE);
}

 int delbword(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n <= 0) { return FALSE; }
    if (0 == (lastflag & CFKILL)) { kdelete(); }
    thisflag |= CFKILL;
    auto endp = curwp->w_dotp;
    auto endo = curwp->w_doto;
    if (FALSE == backchar(FALSE, 1)) { return FALSE; }
    while (n > 0) {
        while (FALSE == inword()) {
            if (FALSE == backchar(FALSE, 1)) { return FALSE; }
        }
        while (FALSE != inword()) {
            if (FALSE == backchar(FALSE, 1)) { goto bckdel; }
        }
        n--;
    }
    if (FALSE == forwchar(FALSE, 1)) { return FALSE; }
bckdel:;
    auto dotp = curwp->w_dotp;
    auto doto = curwp->w_doto;
    auto lp = dotp;
    auto size = 0L;
    while (lp != endp) {
        size += (long)(llength(lp) + 1);
        lp = lforw(lp);
    }
    size += (long)endo;
    size -= (long)doto;
    return ldelete(size, TRUE);
}

 int inword() {
    unsigned int c = 0;
    if (curwp->w_doto == llength(curwp->w_dotp)) { return FALSE; }
    lgetchar((weird(unsigned int, 1))(&c));
    if (iswalpha((wint_t)c) != 0 || iswdigit((wint_t)c) != 0) { return TRUE; }
    return FALSE;
}

 int fillpara(int f, int n) {
    if (0 != ((int)curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (fillcol == 0) {
        mlwrite("No fill column set");
        return FALSE;
    }
    justflag = FALSE;
    gotoeop(FALSE, 1);
    auto eopline = lforw(curwp->w_dotp);
    gotobop(FALSE, 1);
    auto clength = curwp->w_doto;
    if (clength != 0 && lgetc(curwp->w_dotp, 0) == 0x09) { clength = 8; };
    auto wordlen = 0;
    auto dotflag = FALSE;
    unsigned int wbuf[1024];
    auto firstflag = TRUE;
    auto eopflag = FALSE;
    while (FALSE == eopflag) {
        unsigned int c = 0;
        auto bytes = 1;
        if (curwp->w_doto == llength(curwp->w_dotp)) {
            c = 32; // ' '
            if (lforw(curwp->w_dotp) == eopline) { eopflag = TRUE; };
        } else {
            bytes = lgetchar((weird(unsigned int, 1))(&c));
        }
        ldelete((long)bytes, FALSE);
        if (c != 32 && c != 0x09) {
            dotflag = (c == 46); // '.' is 46
            if (wordlen < 1023) { wbuf[wordlen] = c; wordlen++; };
        } else if (0 != wordlen) {
            auto newlength = clength + 1 + wordlen;
            if (newlength <= fillcol) {
                if (FALSE == firstflag) {
                    linsert(1, 32);
                    clength++;
                }
                firstflag = FALSE;
            } else {
                lnewline();
                clength = 0;
            }
            for (auto i=0; i <wordlen; ++i) {
                linsert(1, (int)wbuf[i]);
                clength++;
            }
            if (dotflag) {
                linsert(1, 32);
                clength++;
            }
            wordlen = 0;
        }
    }
    lnewline();
    return TRUE;
}

 int killpara(int f, int n) {
    while (n > 0) {
        gotoeop(FALSE, 1);
        curwp->w_markp = curwp->w_dotp;
        curwp->w_marko = curwp->w_doto;
        gotobop(FALSE, 1);
        curwp->w_doto = 0;
        if (killregion(FALSE, 1) != TRUE) { return FALSE; }
        ldelete(2L, TRUE);
        n--;
    }
    return TRUE;
}

 int wordcount(int f, int n) {
    region r;
    if (getregion((weird(struct region, 1))(&r)) != TRUE) { return FALSE; }
    auto lp = r.r_linep;
    auto offset = r.r_offset;
    auto size = r.r_size;
    auto lastword = FALSE;
    auto nchars = 0L;
    auto nwords = 0L;
    auto nlines = 0;
    while (size > 0) {
        unsigned int ch = 0;
        auto bytes = 1;
        if (offset == llength(lp)) {
            ch = 10; // '\n'
            lp = lforw(lp);
            offset = 0;
            nlines++;
            bytes = 1;
        } else {
            bytes = (int)utf8_to_unicode((weird(unsigned char, 1))lp->l_text, (unsigned int)offset, (unsigned int)llength(lp), (weird(unsigned int, 1))(&ch));
            offset += bytes;
        }
        size -= (long)bytes;
        auto wordflag = (iswalpha((wint_t)ch) != 0 || iswdigit((wint_t)ch) != 0);
        if (TRUE == wordflag && FALSE == lastword) { nwords++; }
        lastword = wordflag;
        nchars++;
    }
    mlwrite((string)"Words %D Chars %D Lines %d", nwords, nchars, nlines);
    return TRUE;
}

// ---- /home/yjlee/namo/random.margo ----
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern  int backline(int f, int n);
extern  int forwline(int f, int n);
extern  void gotobob(int f, int n);
extern  void gotoeob(int f, int n);
extern  int backpage(int f, int n);
extern  int forwpage(int f, int n);
extern  int insspace(int f, int n);
extern  int indent_cancel(int f, int n);
extern  int insert_tab(int f, int n);
extern  int insert_newline(int f, int n);
extern  int completion_menu_command(int f, int n);
extern  int outdent_start_set(int f, int n);
extern  int outdent_end_set(int f, int n);
extern  int indent_start_set(int f, int n);
extern  int indent_end_set(int f, int n);
extern  int g_prefix_handler(int f, int n);
extern  int command_mode_activate_command(int f, int n);
extern  int namo_help_command(int f, int n);
extern  int filesave(int f, int n);
extern  int filefind(int f, int n);
extern  int quit(int f, int n);
extern  int namo_search_engine(int f, int n);
extern  int sed_replace_command(int f, int n);
extern  int cutln_start_copy(int f, int n);
extern  int cutln_end_copy(int f, int n);
extern  int cutln_start_cut(int f, int n);
extern  int cutln_end_cut(int f, int n);
extern  int cutln_cut_current_line(int f, int n);
extern  int yank(int f, int n);
extern  int reserve_jump_1(int f, int n);
extern  int reserve_jump_2(int f, int n);
extern  int reserve_jump_3(int f, int n);
extern  int reserve_jump_4(int f, int n);
extern  int reserve_jump_fallback_1(int f, int n);
extern  int reserve_jump_fallback_2(int f, int n);
extern  int reserve_jump_fallback_3(int f, int n);
extern  int reserve_jump_fallback_4(int f, int n);
extern  int reserve_jump_numeric_mode(int f, int n);

int tabsize = 7;

 static int get_indent(weird(struct line, 1) lp) {
    auto nicol = 0;
    auto len = llength(lp);
    for (auto i=0; i <len; ++i) {
        auto c = lgetc(lp, i);
        if (32 != c && 0x09 != c) { break; }
        if (0x09 == c) {
            nicol = next_tab_stop(nicol, tab_width);
        } else {
            nicol++;
        }
    }
    return nicol;
}

 static void set_indent(int target) {
    auto cur = get_indent(curwp->w_dotp);
    if (cur == target) {
        curwp->w_doto = 0;
        while (curwp->w_doto < llength(curwp->w_dotp)) {
            auto ch = lgetc(curwp->w_dotp, curwp->w_doto);
            if (32 != ch && 0x09 != ch) { break; }
            curwp->w_doto++;
        }
        return;
    }
    curwp->w_doto = 0;
    while (curwp->w_doto < llength(curwp->w_dotp)) {
        auto ch = lgetc(curwp->w_dotp, curwp->w_doto);
        if (32 != ch && 0x09 != ch) { break; }
        ldelchar(1L, FALSE);
    }
    if (target > 0) {
        for (auto i=0; i <target; ++i) {
            linsert(1, 32);
        }
    }
    curwp->w_doto = 0;
    while (curwp->w_doto < llength(curwp->w_dotp)) {
        auto ch2 = lgetc(curwp->w_dotp, curwp->w_doto);
        if (32 != ch2 && 0x09 != ch2) { break; }
        curwp->w_doto++;
    }
}

 int forwdel(int f, int n) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return backdel(f, -n); }
    if (FALSE != f) {
        if (0 == (lastflag & CFKILL)) { kdelete(); }
        thisflag |= CFKILL;
    }
    return ldelchar((long)n, f);
}

 int backdel(int f, int n) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (n < 0) { return forwdel(f, -n); }
    if (FALSE != f) {
        if (0 == (lastflag & CFKILL)) { kdelete(); }
        thisflag |= CFKILL;
    }
    if (TRUE == backchar(f, n)) { return ldelchar((long)n, f); }
    return FALSE;
}

 int killtext(int f, int n) {
    if (0 != (curbp->b_mode & MDVIEW)) { return rdonly(); }
    if (0 == (lastflag & CFKILL)) { kdelete(); }
    thisflag |= CFKILL;
    auto chunk = 0L;
    if (FALSE == f) {
        chunk = (long)(llength(curwp->w_dotp) - curwp->w_doto);
        if (0L == chunk) {
            chunk = 1L;
        }
    } else if (0 == n) {
        chunk = (long)curwp->w_doto;
        curwp->w_doto = 0;
    } else if (n > 0) {
        chunk = (long)(llength(curwp->w_dotp) - curwp->w_doto + 1);
        auto nextp = lforw(curwp->w_dotp);
        while (n > 1) {
            if (nextp == curbp->b_linep) { return FALSE; }
            chunk += (long)(llength(nextp) + 1);
            nextp = lforw(nextp);
            n--;
        }
    } else {
        mlwrite("neg kill");
        return FALSE;
    }
    return ldelete(chunk, TRUE);
}

 int adjustmode(int kind, int global) {
    char prompt[64];
    char cbuf[1024];
    if (0 != global) {
        strcpy(prompt, "Global mode to ");
    } else {
        strcpy(prompt, "Mode to ");
    }
    if (TRUE == kind) {
        strcat(prompt, "add: ");
    } else {
        strcat(prompt, "delete: ");
    }
    auto status = minibuf_input(prompt, (weird(char, 1))cbuf, 1023);
    if (TRUE != status) { return status; }
    for (auto i=0; i <(int)strlen(cbuf); ++i) {
        if (cbuf[i] >= 'a' && cbuf[i] <= 'z') {
            cbuf[i] = (char)((int)cbuf[i] - 32);
        }
    }
    for (auto i=0; i <9; ++i) { // NUMMODES
        if (0 == strcmp((string)cbuf, modename[i])) {
            if (TRUE == kind) {
                if (0 != global) {
                    gmode |= (1 << i);
                } else {
                    curbp->b_mode |= (1 << i);
                }
            } else {
                if (0 != global) {
                    gmode &= ~(1 << i);
                } else {
                    curbp->b_mode &= ~(1 << i);
                }
            }
            if (0 == global) { upmode(); }
            mlerase();
            return TRUE;
        }
    }
    mlwrite("No such mode!");
    return FALSE;
}

 int setemode(int f, int n) { return adjustmode(TRUE, FALSE); }
 int delmode(int f, int n) { return adjustmode(FALSE, FALSE); }
 int setgmode(int f, int n) { return adjustmode(TRUE, TRUE); }
 int delgmode(int f, int n) { return adjustmode(FALSE, TRUE); }

 int clrmes(int f, int n) {
    mlforce("");
    return TRUE;
}

 int writemsg(int f, int n) {
    char buf[1024];
    char nbuf[2048];
    auto status = minibuf_input("Message to write: ", (weird(char, 1))buf, 1023);
    if (TRUE != status) { return status; }
    auto sp = 0;
    auto np = 0;
    while (0 != buf[sp]) {
        nbuf[np] = buf[sp];
        np++;
        if ('%' == buf[sp]) {
            nbuf[np] = (char)'%';
            np++;
        }
        sp++;
    }
    nbuf[np] = (char)0;
    mlforce((string)nbuf);
    return TRUE;
}

 int getfence(int f, int n) {
    auto oldlp = curwp->w_dotp;
    auto oldoff = curwp->w_doto;
    auto ch = 0;
    if (oldoff == llength(oldlp)) {
        ch = 10; // '\n'
    } else {
        ch = lgetc(oldlp, oldoff);
    }
    auto ofence = 0;
    auto sdir = FORWARD;
    if (ch == 40) { // '('
        ofence = 41; // ')'
        sdir = FORWARD;
    } else if (ch == 123) { // '{'
        ofence = 125; // '}'
        sdir = FORWARD;
    } else if (ch == 91) { // '['
        ofence = 93; // ']'
        sdir = FORWARD;
    } else if (ch == 41) { // ')'
        ofence = 40; // '('
        sdir = REVERSE;
    } else if (ch == 125) { // '}'
        ofence = 123; // '{'
        sdir = REVERSE;
    } else if (ch == 93) { // ']'
        ofence = 91; // '['
        sdir = REVERSE;
    } else {
        TTbeep();
        return FALSE;
    }
    auto count = 1;
    if (sdir == REVERSE) {
        backchar(FALSE, 1);
    } else {
        forwchar(FALSE, 1);
    }
    while (count > 0) {
        auto c = 0;
        if (curwp->w_doto == llength(curwp->w_dotp)) {
            c = 10; // '\n'
        } else {
            c = lgetc(curwp->w_dotp, curwp->w_doto);
        }
        if (c == ch) {
            count++;
        } else if (c == ofence) {
            count--;
        }
        if (sdir == FORWARD) {
            forwchar(FALSE, 1);
        } else {
            backchar(FALSE, 1);
        }
        if (0 != boundry(curwp->w_dotp, curwp->w_doto, sdir)) { break; }
    }
    if (0 == count) {
        if (sdir == FORWARD) {
            backchar(FALSE, 1);
        } else {
            forwchar(FALSE, 1);
        }
        curwp->w_flag = (char)((int)curwp->w_flag | WFMOVE);
        return TRUE;
    }
    curwp->w_dotp = oldlp;
    curwp->w_doto = oldoff;
    TTbeep();
    return FALSE;
}

 int istring(int f, int n) {
    char tstring[1024];
    auto status = mlreplyt("String to insert<META>: ", (weird(char, 1))tstring, 1023, metac);
    if (TRUE != status) { return status; }
    if (FALSE == f) {
        n = 1;
    }
    if (n < 0) {
        n = -n;
    }
    while (n > 0) {
        status = linstr((string)tstring);
        if (TRUE != status) { break; }
        n--;
    }
    return status;
}

 int ovstring(int f, int n) {
    char tstring[1024];
    auto status = mlreplyt("String to overwrite<META>: ", (weird(char, 1))tstring, 1023, metac);
    if (TRUE != status) { return status; }
    if (FALSE == f) {
        n = 1;
    }
    if (n < 0) {
        n = -n;
    }
    while (n > 0) {
        status = lover((string)tstring);
        if (TRUE != status) { break; }
        n--;
    }
    return status;
}

// ---- /home/yjlee/namo/display.margo ----

/* @style c */ {
void vtinit(void);
void vttidy(void);
void vtmove(int row, int col);
void vtputc(int c);
void vteeol(void);
int update(int force);
int upscreen(int f, int n);
int updupd(int force);
void movecursor(int row, int col);
void getscreensize(int *widthp, int *heightp);
int newscreensize(int h, int w);
void sizesignal(int signr);
}

// ---- /home/yjlee/namo/namo_core.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int NAMO_PATH_MAX = 4096;
const int NAMO_SLOT_POOL = 64;
const int NAMO_SLOT_VISIBLE = 4;
const int NAMO_LAMP_OFF = 0;
const int NAMO_LAMP_WARN = 1;
const int NAMO_LAMP_ERROR = 2;

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
typedef struct namo_config namo_config;

namo_config namo_cfg;

char file_reserve[NAMO_SLOT_POOL][NAMO_PATH_MAX];
int should_redraw_underbar = FALSE;
static int lamp_state = NAMO_LAMP_OFF;

// Missing symbols reported by linker
string namo_message_prefix = "Message: ";
char sres[128] = "";

 void namo_request_underbar_redraw() {
    should_redraw_underbar = TRUE;
}

 void namo_init() {
    namo_cfg.hint_bar = TRUE;
    namo_cfg.no_function_slot = FALSE;
    namo_cfg.warning_lamp = TRUE;
    namo_cfg.nonr = FALSE;
    namo_cfg.soft_tab = TRUE;
    namo_cfg.soft_tab_width = 4;
    namo_cfg.autocomplete = TRUE;
    namo_cfg.use_lsp = FALSE;
    namo_cfg.case_sensitive_default = FALSE;
    mystrscpy(namo_cfg.warning_format, "--W", 16);
    mystrscpy(namo_cfg.error_format, "--E", 16);
    mystrscpy(namo_cfg.help_key, "F1", 16);
    mystrscpy(namo_cfg.help_language, "en", 16);
    mystrscpy(namo_cfg.message_prefix, "Message: ", 64);
    namo_message_prefix = (string)namo_cfg.message_prefix;
}

 void namo_apply_config() {
    // Apply config logic
}

 void namo_set_lamp(int state) {
    lamp_state = state;
}

 int namo_current_lamp() {
    return lamp_state;
}

 string namo_lamp_label() {
    if (lamp_state == NAMO_LAMP_WARN) { return (string)namo_cfg.warning_format; }
    if (lamp_state == NAMO_LAMP_ERROR) { return (string)namo_cfg.error_format; }
    return "";
}

 int namo_text_rows() {
    auto rows = (int)term->t_nrow;
    if (TRUE == namo_cfg.hint_bar) {
        rows = rows - 2;
    }
    return rows;
}

 int namo_text_cols() {
    auto cols = (int)term->t_ncol;
    if (FALSE == namo_cfg.nonr) {
        cols = cols - 6;
    }
    return cols;
}

 int namo_hint_top_row() {
    return (int)term->t_nrow - 1;
}

 int namo_hint_bottom_row() {
    return (int)term->t_nrow;
}

 void namo_notify_message(string text) {
    mlwrite(text);
}

 void namo_handle_closed_file(string path) {
    // Handle closed file logic
}

 void namo_cleanup() {
    // Cleanup logic
}

 bool namo_help_is_active() {
    return FALSE;
}

 void namo_help_render() {
    // Render when ready
}

 int namo_help_command(int f, int n) {
    return FALSE;
}

 int namo_help_handle_key(int key) {
    return FALSE;
}

// ---- /home/yjlee/namo/ebind.margo ----

key_tab keytab[2048] = {
    { SPEC | (int)'A', (weird(void, 1))backline },
    { SPEC | (int)'B', (weird(void, 1))forwline },
    { SPEC | (int)'C', (weird(void, 1))forwchar },
    { SPEC | (int)'D', (weird(void, 1))backchar },
    { SPEC | (int)'H', (weird(void, 1))gotobob },
    { SPEC | (int)'F', (weird(void, 1))gotoeob },
    { SPEC | (int)'5', (weird(void, 1))backpage },
    { SPEC | (int)'6', (weird(void, 1))forwpage },
    { SPEC | (int)'L', (weird(void, 1))insspace },
    { 0x7F, (weird(void, 1))indent_cancel },
    { SPEC | 0x7F, (weird(void, 1))forwdel },
    { CONTROL | (int)'I', (weird(void, 1))insert_tab },
    { CONTROL | (int)'M', (weird(void, 1))insert_newline },
    { CONTROL | (int)'@', (weird(void, 1))completion_menu_command },
    { CONTROL | (int)'H', (weird(void, 1))outdent_start_set },
    { CONTROL | SHIFT | (int)'H', (weird(void, 1))outdent_end_set },
    { CONTROL | (int)'J', (weird(void, 1))indent_start_set },
    { CONTROL | SHIFT | (int)'J', (weird(void, 1))indent_end_set },
    { (int)'g', (weird(void, 1))g_prefix_handler },
    { CONTROL | (int)'V', (weird(void, 1))command_mode_activate_command },
    { SPEC | (int)'P', (weird(void, 1))namo_help_command },
    { SPEC | (int)'Q', (weird(void, 1))filesave },
    { CONTROL | (int)'S', (weird(void, 1))filesave },
    { SPEC | (int)'R', (weird(void, 1))filefind },
    { CONTROL | (int)'O', (weird(void, 1))filefind },
    { SPEC | (int)'S', (weird(void, 1))quit },
    { CONTROL | (int)'Q', (weird(void, 1))quit },
    { SPEC | (int)'U', (weird(void, 1))namo_search_engine },
    { CONTROL | (int)'F', (weird(void, 1))namo_search_engine },
    { CONTROL | (int)'R', (weird(void, 1))sed_replace_command },
    { SPEC | (int)'W', (weird(void, 1))cutln_start_copy },
    { SPEC | SHIFT | (int)'W', (weird(void, 1))cutln_end_copy },
    { CONTROL | (int)'W', (weird(void, 1))cutln_start_copy },
    { CONTROL | SHIFT | (int)'W', (weird(void, 1))cutln_end_copy },
    { SPEC | (int)'X', (weird(void, 1))cutln_start_cut },
    { SPEC | SHIFT | (int)'X', (weird(void, 1))cutln_end_cut },
    { CONTROL | (int)'X', (weird(void, 1))cutln_start_cut },
    { CONTROL | SHIFT | (int)'X', (weird(void, 1))cutln_end_cut },
    { CONTROL | (int)'K', (weird(void, 1))cutln_cut_current_line },
    { CONTROL | SHIFT | (int)'K', (weird(void, 1))cutln_end_cut },
    { CONTROL | (int)'Y', (weird(void, 1))yank },
    { SPEC | (int)'Y', (weird(void, 1))yank },
    { META | CONTROL | (int)'8', (weird(void, 1))yank },
    { SPEC | (int)'`', (weird(void, 1))reserve_jump_1 },
    { SPEC | (int)'a', (weird(void, 1))reserve_jump_2 },
    { SPEC | (int)'{', (weird(void, 1))reserve_jump_3 },
    { SPEC | (int)'}', (weird(void, 1))reserve_jump_4 },
    { META | CONTROL | (int)'9', (weird(void, 1))reserve_jump_fallback_1 },
    { META | CONTROL | (int)'0', (weird(void, 1))reserve_jump_fallback_2 },
    { META | CONTROL | (int)'-', (weird(void, 1))reserve_jump_fallback_3 },
    { META | CONTROL | (int)'=', (weird(void, 1))reserve_jump_fallback_4 },
    { META | CONTROL | (int)'1', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'2', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'3', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'4', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'5', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'6', (weird(void, 1))reserve_jump_numeric_mode },
    { META | CONTROL | (int)'7', (weird(void, 1))reserve_jump_numeric_mode },
    { CONTROL | (int)'G', (weird(void, 1))gotoline },
    { 0, NULL }
};

// ---- /home/yjlee/namo/bind.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

 weird(void, 1) getbind(int c) {
    auto i = 0;
    while (keytab[i].k_code != 0) {
        if (keytab[i].k_code == c) return keytab[i].k_fp;
        i++;
    }
    return NULL;
}

 int bindtokey(int f, int n) {
    // bind to key logic
    return TRUE;
}

 int unbindkey(int f, int n) {
    // unbind key logic
    return TRUE;
}

// ---- /home/yjlee/namo/namo.margo ----
// namo.margo
// This file is now primarily for core logic that isn't in other modules.
// Imports are handled by the main entry point.

// ---- /home/yjlee/namo/main.margo ----
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


 void namo_refresh_ui() {
    if (namo_help_is_active()) {
        namo_help_render();
    } else {
        update(FALSE);
        // completion_dropdown_render()
    }
}

 void edinit(string bname) {
    auto bp = bfind(bname, TRUE, 0);
    blistp = bfind("*List*", TRUE, BFINVS);
    auto wp = (weird(struct window, 1))alloc(sizeof(window));
    if (bp == NULL || wp == NULL || blistp == NULL) { exit(1); }
    curbp = bp;
    bp->b_mode |= MDSOFTWRAP;
    curwp = wp;
    wp->w_bufp = bp;
    bp->b_nwnd = 1;
    wp->w_linep = bp->b_linep;
    wp->w_dotp = bp->b_linep;
    wp->w_doto = 0;
    wp->w_markp = NULL;
    wp->w_marko = 0;
    wp->w_force = 0;
    wp->w_flag = (char)(WFMODE | WFHARD);
}

 int execute(int c, int f, int n) {
    // Basic execute logic
    return TRUE;
}

 int main(int argc, weird(string, 1) argv) {
    char bname[1024] = "main";
    
    // Process arguments...

    globals_init_tables();
    
    vtinit();
    edinit((string)bname);
    
    while (true) {
        namo_refresh_ui();
        auto c = getcmd();
        if (c == 0x18) { // Ctrl+X
            auto c2 = getcmd();
            if (c2 == 0x03) { break; } // Ctrl+C to exit
        }
        execute(c, FALSE, 1);
    }
    
    vttidy();
    return 0;
}
