#ifndef NEO_IV_CLOCK_MINI_FONT_H
#define NEO_IV_CLOCK_MINI_FONT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

/* 字库最大容纳的字符数 */
#define MINI_FONT_CHAR_CNT       256

/* 哈希桶的数量（取略大于字符数的质数以减少冲突） 
NextPrime(MINI_FONT_CHAR_CNT * r)
r = 1.3~1.5
*/
#define MINI_FONT_HASH_BUCKETS   347

/* 字符类型枚举 */
typedef enum _mini_font_type_t{
  MINI_FONT_TYPE_NONE = 0,
  MINI_FONT_TYPE_ASCII_6X8,
  MINI_FONT_TYPE_ASCII_8X16,
  MINI_FONT_TYPE_CHINESE_16X16,
  MINI_FONT_TYPE_CNT
} mini_font_type_t;

typedef struct _mini_font_size_t {
  uint8_t w;             // 字符宽
  uint8_t h;             // 字符高
} mini_font_size_t;

/* 哈希链表节点：现在仅保存外部点阵的指针 */
typedef struct _mini_font_node_t {
    wchar_t unicode_char;               // Unicode 键值
    const uint8_t *font_ptr;            // 指向外部已经实现好的点阵数据指针
    mini_font_type_t type;              // 字符类型
    struct _mini_font_node_t * next;    // 链表指针（指向静态池中的下一个节点）
} mini_font_node_t;

typedef bool (*MINI_FONT_INIT_FUN)(mini_font_node_t *);

typedef struct _mini_font_lib_t {
  const char * lib_name;               // 字库名
  mini_font_node_t * font_table;       // 字库数据
  MINI_FONT_INIT_FUN font_init;        // 字库初始化函数
} mini_font_lib_t;

/* 外部接口声明 */
bool mini_font_init(void);
const uint8_t* mini_font_lookup(wchar_t unicode_char, mini_font_type_t type, uint8_t * w, uint8_t * h);

#endif // NEO_IV_CLOCK_MINI_FONT_H
