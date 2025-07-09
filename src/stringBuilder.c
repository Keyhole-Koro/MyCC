#include "stringBuilder.h"

void sb_init(StringBuilder *sb) {
    sb->cap = 1024;
    sb->len = 0;
    sb->buf = malloc(sb->cap);
    sb->buf[0] = '\0';
}
void sb_append(StringBuilder *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (sb->len + need + 1 > sb->cap) {
        sb->cap = (sb->len + need + 1) * 2;
        sb->buf = realloc(sb->buf, sb->cap);
    }
    va_start(ap, fmt);
    vsprintf(sb->buf + sb->len, fmt, ap);
    va_end(ap);
    sb->len += need;
}
char *sb_dump(StringBuilder *sb) {
    return sb->buf;
}
