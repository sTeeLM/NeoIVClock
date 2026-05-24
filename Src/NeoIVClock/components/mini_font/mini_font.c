#include "mini_font.h"
#include "mini_font_ascii.h"
#include "mini_font_chinese.h"
#include "logger.h"

static const char * TAG = "MINI_FONT";


// 所有的字库，添加新的字库条目，需要改MINI_FONT_LIB_SIZE
#define MINI_FONT_LIB_SIZE 3
const mini_font_lib_t mini_font_lib[MINI_FONT_LIB_SIZE] = 
{
  {"ASCII 6X8", mini_font_ascii_font_6x8, mini_font_ascii_font_6x8_init},
  {"ASCII 8X16", mini_font_ascii_font_8x16, mini_font_ascii_font_8x16_init},
  {"CHINESE 8X16", mini_font_chinese_font_16x16, mini_font_chinese_font_16x16_init}
};

const mini_font_size_t mini_font_size[MINI_FONT_TYPE_CNT] = 
{
  {0,  0},  // MINI_FONT_TYPE_NONE
  {6,  8},  // MINI_FONT_TYPE_ASCII_6X8
  {8,  16}, // MINI_FONT_TYPE_ASCII_8X16 
  {16, 16}, // MINI_FONT_TYPE_CHINESE_16X16
};

// 哈希表
static EXT_RAM_BSS_ATTR mini_font_node_t * mini_font_hash_table[MINI_FONT_HASH_BUCKETS];

/* 联合键哈希函数：融合字符码点和类型 */
static uint32_t mini_font_hash(wchar_t unicode_char, mini_font_type_t type) 
{
  // 经典的 FNV-1a 简化版或位拼接混淆
  // 将 type 的值混合到 Unicode 码点中，确保相同字符、不同 type 映射到不同桶中
  uint32_t hash = (uint32_t)unicode_char;
  hash = (hash ^ (uint32_t)type) * 16777619UL; 
  return hash % MINI_FONT_HASH_BUCKETS;
}

/* 注册字符：现在根据联合键进行排重和插入 */
static bool mini_font_add_hash(mini_font_node_t * node) 
{
  uint32_t bucket = 0;
  mini_font_node_t *curr = NULL;

  if (node == NULL) {
    return false;
  }
  
  bucket = mini_font_hash(node->unicode_char, node->type);
  curr = mini_font_hash_table[bucket];

  // 联合查找：必须同时匹配 unicode_char 和 type
  while (curr != NULL) {
    if (curr->unicode_char == node->unicode_char && curr->type == node->type) {
      // 找到完全一致的记录，报错
      NEO_LOGE(TAG, "dup record for char = %04x type = %d", node->unicode_char, node->type);
      return false;
    }
    curr = curr->next;
  }

  // 头插法入桶
  node->next = mini_font_hash_table[bucket];
  mini_font_hash_table[bucket] = node;

  NEO_LOGD(TAG, "add char [%04x][%d]", node->unicode_char, node->type);

  return true;
}

/* 根据联合键查找 font, 并返回大小 */
const uint8_t* mini_font_lookup(wchar_t unicode_char, mini_font_type_t type, uint8_t * w, uint8_t * h) 
{
  uint32_t bucket = mini_font_hash(unicode_char, type);
  mini_font_node_t *curr = mini_font_hash_table[bucket];

  if(type > MINI_FONT_TYPE_CNT - 1) {
    NEO_LOGE(TAG, "invalid type = %d", type);
    if(w) *w = mini_font_size[type].w;
    if(h) *h = mini_font_size[type].h;
    return NULL;
  }

  if(w) *w = mini_font_size[type].w;
  if(h) *h = mini_font_size[type].h;

  // 联合校验匹配
  while (curr != NULL) {
    if (curr->unicode_char == unicode_char && curr->type == type) {
      return curr->font_ptr; // 精确匹配成功
    }
    curr = curr->next;
  }
  NEO_LOGE(TAG, "miss record for char = 0x%04x type = %d", unicode_char, type);
  return NULL; // 没找到
}

/* 初始化字库 */
bool mini_font_init(void)
{
  mini_font_node_t * p = NULL;
  uint32_t i, char_cnt;
  
  for (i = 0; i < MINI_FONT_HASH_BUCKETS; i++) {
      mini_font_hash_table[i] = NULL;
  }

  for(i = 0 ; i < MINI_FONT_LIB_SIZE ; i ++) {
    p = mini_font_lib[i].font_table;
    NEO_LOGI(TAG, "add lib %s..", mini_font_lib[i].lib_name);
    if(!mini_font_lib[i].font_init(p)) {
      NEO_LOGE(TAG, "init lib %s failed", mini_font_lib[i].lib_name);
    }
    char_cnt = 0;
    while(p->font_ptr) {
      if(!mini_font_add_hash(p)) {
        NEO_LOGE(TAG, "add lib %s failed", mini_font_lib[i].lib_name);
        return false;
      }
      p ++;
      char_cnt ++;
    }
    NEO_LOGI(TAG, "add lib %s ok, total %d char added", mini_font_lib[i].lib_name, char_cnt);
  }
  return true;
}