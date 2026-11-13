/* Copyright 2019 sekigon-gonnoc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H
#include "bmp.h"
#include "bmp_custom_keycode.h"
#include "keycode_str_converter.h"

// 追加
#include <string.h>
#include "keymap_jp.h"

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

extern keymap_config_t keymap_config;
// #ifdef RGBLIGHT_ENABLE
// //Following line allows macro to read current RGB settings
// extern rgblight_config_t rgblight_config;
// #endif

extern uint8_t is_master;

// 遅延入力処理用定数と変数
// レイヤーキーとの同時打鍵を実現するために一時的な入力内容を静的変数に保持する
// DELAY_TIME以内であればレイヤーキーの押下が後でも反映される
#define DELAY_TIME  75 // 遅延入力有効時間 (ms)
static uint16_t key_timer; // 遅延入力用タイマー
static uint16_t tap_timer; // タップ用タイマー
static uint16_t delay_registered_code; // 遅延入力されたキーコード
static uint8_t delay_mat_row; // 遅延入力された行
static uint8_t delay_mat_col; // 遅延入力された列
static bool delay_key_stat; // 遅延入力キーがあるときにtrue
static bool delay_key_pressing; // 延入力キーが押されている間はtrue
static bool tapping_key;

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.
// Layer names don't all need to be of the same length, obviously, and you can also skip them
// entirely and just use numbers.

bool RGBAnimation = false; //Flag for LED Layer color Refresh.

// Defines the keycodes used by our macros in process_record_user
enum custom_keycodes {
    QWERTY = BMP_SAFE_RANGE, 
    EISU,
    KANA,
    RGBRST,
    RGBOFF,
    RGB1,
    RGB2,
    RGB3,
    OPT_TAP_SP,
    DESKTOP,
    MAC,
    WIN,
};

const key_string_map_t custom_keys_user =
{
    .start_kc = QWERTY,
    .end_kc = WIN,
    .key_strings = "QWERTY\0"
    "EISU\0"
    "KANA\0"
    "RGBRST\0"
    "RGBOFF\0"
    "RGB1\0"
    "RGB2\0"
    "RGB3\0"
    "OPT_TAP_SP\0"
    "DESKTOP\0"
    "MAC\0"
    "WIN\0"
};

enum layers {
    _BASE = 0, _OPT = 1, _FUNC = 2, _SYM = 3, _NUM = 4
};

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Base
   * ,-----------------------------------------.         
   * |  C+z |   ;  |   [  |   (  |   <  |   {  |         
   * |------+------+------+------+------+------|         
   * | KANA |   P  |   K  |   R  |   A  |   F  |         
   * |------+------+------+------+------+------|         
   * |  BS  |   D  |   T  |   H  |   E  |   O  |         
   * |------+------+------+------+------+------+------|
   * | Shift|   Y  |   S  |   N  |   I  |   U  |Space |
   * |------+------+------+------+------+------+------|
   * | Ctrl | Alt  | Gui  | Sym  | Num  | OPT  | Ent  |
   * `------------------------------------------------'
   */
  [_BASE] = LAYOUT( \
      LCTL(JP_Z),    JP_SCLN,       JP_LBRC,       JP_LPRN,   JP_LT,     JP_LCBR,            \
      KANA,          JP_P,          JP_K,          JP_R,      JP_A,      JP_F,               \
      KC_BSPC,       JP_D,          JP_T,          JP_H,      JP_E,      JP_O,               \
      OSM(MOD_LSFT), JP_Y,          JP_S,          JP_N,      JP_I,      JP_U,       KC_SPC, \
      OSM(MOD_LCTL), OSM(MOD_LALT), OSM(MOD_LGUI), MO(_SYM),  MO(_NUM),  OPT_TAP_SP, KC_ENT  \
      ),
  /* Opt
   * ,-----------------------------------------.       
   * |  Esc |  :   |  ]   |  )   |  >   |  }   |       
   * |------+------+------+------+------+------|       
   * |  EISU|   J  |   M  |   B  |   '  |  Tab |       
   * |------+------+------+------+------+------|       
   * |   .  |   V  |   C  |   L  |   Z  |  Q   |       
   * |------+------+------+------+------+------+------|
   * |      |   X  |   G  |   W  |   -  |  Del | Esc  |
   * |------+------+------+------+------+------+------|
   * |      |      |      |   ,  | DTOP |      |      |
   * `------------------------------------------------'
   */
  [_OPT] = LAYOUT( \
      KC_ESC,  JP_COLN,JP_RBRC, JP_RPRN,JP_GT,     JP_RCBR,         \
      EISU,    JP_J,   JP_M,    JP_B,   JP_QUOT,   KC_TAB,          \
      KC_DOT,  JP_V,   JP_C,    JP_L,   JP_Z,      JP_Q,            \
      _______, JP_X,   JP_G,    JP_W,   JP_MINS,  KC_DEL,  KC_ESC, \
      _______, _______,_______, JP_COMM,DESKTOP,   _______, _______ \
      ),
  /* Func
   * ,-----------------------------------------.       
   * |RGBRST|  Hue |      |  RST |  Mac |  Win |       
   * |------+------+------+------+------+------|       
   * | RGB1 | VAL+ |  F7  |  F8  |  F9  |      |       
   * |------+------+------+------+------+------|       
   * | RGB2 | VAL- |  F4  |  F5  |  F6  | F12  |       
   * |------+------+------+------+------+------+------|
   * | RGB3 |  F10 |  F1  |  F2  |  F3  | F11  |      |
   * |------+------+------+------+------+------+------|
   * |RGBOFF|      |      |      |      |      |      |
   * `------------------------------------------------'
   */
  [_FUNC] = LAYOUT( \
      AD_WO_L,RGB_HUI, _______, RESET,   MAC,     WIN,               \
      ADV_ID0,  RGB_VAI, KC_F7,   KC_F8,   KC_F9,   _______,           \
      ADV_ID1,  RGB_VAD, KC_F4,   KC_F5,   KC_F6,   KC_F12,            \
      ADV_ID2,  KC_F10,  KC_F1,   KC_F2,   KC_F3,   KC_F11,   _______, \
      DELBNDS,  _______, _______, _______, _______, _______,  _______  \
      ),
  /* Sym
   * ,-----------------------------------------.       
   * |  Ins |  GRV |      |  PU  |  PD  |   ^  |       
   * |------+------+------+------+------+------|       
   * |      |   \  |   #  |   =  |   ?  |   %  |       
   * |------+------+------+------+------+------|       
   * |      |   $  |  upA |   @  |   !  |   |  |       
   * |------+------+------+------+------+------+------|
   * |  CL  |  <-  |  dwA |  ->  |   _  |   &  |      |
   * |------+------+------+------+------+------+------|
   * |      |      |  PS  |      |   ~  |      |      |
   * `------------------------------------------------'
   */
  [_SYM] = LAYOUT( \
      KC_INS,  JP_GRV,  _______, KC_PGUP, KC_PGDN, JP_CIRC,         \
      _______, JP_BSLS, JP_HASH, JP_EQL,  JP_QUES, JP_PERC,         \
      _______, JP_DLR,  KC_UP,   JP_AT,   JP_EXLM, JP_PIPE,         \
      S(JP_CAPS), KC_LEFT, KC_DOWN, KC_RIGHT,JP_UNDS, JP_AMPR, _______,\
      _______, KC_PSCR, _______,  _______, JP_TILD, _______, _______ \
      ),
  /* NUM
   * ,-----------------------------------------.       
   * |      |      | Func | home |  End |      |       
   * |------+------+------+------+------+------|       
   * |      |   *  |  7   |  8   |  9   |  -   |       
   * |------+------+------+------+------+------|       
   * |  .   |   /  |  4   |  5   |  6   |  +   |       
   * |------+------+------+------+------+------+------|
   * |  LN  |  0   |  1   |  2   |  3   |C+S+F1|      |
   * |------+------+------+------+------+------+------|
   * |      |      |      |  ,   |      |      |      |
   * `------------------------------------------------'
   */
  [_NUM] = LAYOUT( \
      BATT_LV,  _______, OSL(_FUNC), KC_HOME, KC_END,  LCTL(KC_F),     \
      _______,  JP_ASTR, KC_P7,      KC_P8,   KC_P9,   JP_MINS,        \
      KC_PDOT,  JP_SLSH, KC_P4,      KC_P5,   KC_P6,   JP_PLUS,        \
      KC_NLCK,  KC_P0,   KC_P1,      KC_P2,   KC_P3,   LCTL(S(KC_F1)), _______, \
      _______,  _______, KC_PDOT,    JP_COMM, _______, _______,        _______ \
      )
};

uint32_t keymaps_len() {
  return 32;
}
// 追加

// define variables for reactive RGB
//bool TOG_STATUS = false;
// int RGB_current_mode;

// void persistent_default_layer_set(uint16_t default_layer) {
//   eeconfig_update_default_layer(default_layer);
//   default_layer_set(default_layer);
// }


bool find_mairix(uint16_t keycode, uint8_t *row, uint8_t *col){
  for(uint8_t i=0; i<MATRIX_ROWS; i++){
    for(uint8_t j=0; j<MATRIX_COLS; j++){
      if( pgm_read_word(&(keymaps[_BASE][i][j]))==keycode){
        *row = i;
        *col = j;
        return true;
      }
    }
  }
  return false;
}

void unregister_delay_code(void){
  if(delay_registered_code){
    unregister_code(delay_registered_code);
    if (delay_registered_code & QK_LSFT){
      unregister_code(KC_LSFT);
    }
    if (delay_registered_code & QK_LCTL){
      unregister_code(KC_LCTL);
    }
    if (delay_registered_code & QK_LALT){
      unregister_code(KC_LALT);
    }
    if (delay_registered_code & QK_LGUI){
      unregister_code(KC_LGUI);
    }
    delay_registered_code=0;
  }
}

void register_delay_code(uint8_t layer){
  if(delay_key_stat){
    unregister_delay_code();

    uint16_t code = pgm_read_word(&(keymaps[layer][delay_mat_row][delay_mat_col]));
    if (code & QK_LSFT){
      register_code(KC_LSFT);
    }
    if (code & QK_LCTL){
      register_code(KC_LCTL);
    }
    if (code & QK_LALT){
      register_code(KC_LALT);
    }
    if (code & QK_LGUI){
      register_code(KC_LGUI);
    }
    register_code(code);
    delay_registered_code = code;
    delay_key_stat = false;
    tapping_key = true;
  }
}

///////////////////////////////
// #ifdef RGBLIGHT_ENABLE
// struct keybuf {
//   char col, row;
//   char frame;
// };
// struct keybuf keybufs[256];
// unsigned char keybuf_begin, keybuf_end;

// int col, row;
// #endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  bool continue_process = process_record_user_bmp(keycode, record);
  if (continue_process == false)
  {
    return false;
  }
  // #ifdef RGBLIGHT_ENABLE
  //   col = record->event.key.col;
  //   row = record->event.key.row;
  //   if (record->event.pressed && ((row < 5 && is_keyboard_master()) || (row >= 5 && !is_keyboard_master()))) {
  //     int end = keybuf_end;
  //     keybufs[end].col = col;
  //     keybufs[end].row = row % 5;
  //     keybufs[end].frame = 0;
  //     keybuf_end ++;
  //   }
  // #endif

  if(tap_timer&&keycode!=OPT_TAP_SP){
    tapping_key = true;
  }

  if(keycode==delay_registered_code){
      if (!record->event.pressed){
        unregister_delay_code();
      }
  }

  switch (keycode) {
    // 追加
    case JP_SCLN:
    case JP_LBRC:
    case JP_LPRN:
    case JP_LT:
    case JP_LCBR:
    case JP_P:
    case JP_K:
    case JP_R:
    case JP_A:
    case JP_F:
    case KC_BSPC:
    case JP_D:
    case JP_T:
    case JP_H:
    case JP_E:
    case JP_O:
    case JP_Y:
    case JP_S:
    case JP_N:
    case JP_I:
    case JP_U:
    case LCTL(JP_Z):
    case KC_SPC:
      if (record->event.pressed) {
        register_delay_code(_BASE);
        unregister_delay_code();
        if(find_mairix(keycode, &delay_mat_row, &delay_mat_col)){
          key_timer = timer_read();
          delay_key_stat = true;
          delay_key_pressing = true;
        }
      }else{
        delay_key_pressing = false;
      }
      return false;
      break;
    case OPT_TAP_SP:
      if (record->event.pressed) {
        tapping_key = false;
        register_delay_code(_OPT);
        unregister_delay_code();
        layer_on(_OPT);
        tap_timer = timer_read();
      }else{
        layer_off(_OPT);
        if(tapping_key==false && timer_elapsed(tap_timer) < TAPPING_TERM){
          SEND_STRING(" ");
        }
        tap_timer = 0;
      }
      return false;
      break;
    case EISU:
      if (record->event.pressed) {
        if (is_mac_mode()) {
          register_code(KC_LANG2);
        }else{
          tap_code16(LALT(KC_GRAVE));
        }
      } else {
        unregister_code(KC_LANG2);
      }
      return false;
      break;
    case KANA:
      if (record->event.pressed) {
        if (is_mac_mode()) {
          register_code(KC_LANG1);
        }else{
          tap_code16(LALT(KC_GRAVE));
        }
      } else {
        unregister_code(KC_LANG1);
      }
      return false;
      break;
    case DESKTOP:
      if (record->event.pressed) {
        if (is_mac_mode()) {
          register_code(KC_F11);
        }else{
          SEND_STRING(SS_LGUI("d"));
        }
      } else {
        unregister_code(KC_F11);
      }
      return false;
      break;
    // case RGBRST:
    //   #ifdef RGBLIGHT_ENABLE
    //     if (record->event.pressed) {
    //       eeconfig_update_rgblight_default();
    //       rgblight_enable();
    //       RGB_current_mode = rgblight_config.mode;
    //       RGBAnimation = false;
    //     }
    //   #endif
    //   break;
    // case RGBOFF:
    //   #ifdef RGBLIGHT_ENABLE
    //     if (record->event.pressed) {
    //       rgblight_disable();
    //     }
    //   #endif
    //   break;
    // case RGB1:
    //   #ifdef RGBLIGHT_ENABLE
    //     if (record->event.pressed) {
    //       RGBAnimation = true;
    //       rgblight_mode(RGBLIGHT_MODE_RAINBOW_MOOD);
    //       RGB_current_mode = rgblight_config.mode;
    //     }
    //   #endif
    //   break;
    // case RGB2:
    //   #ifdef RGBLIGHT_ENABLE
    //     if (record->event.pressed) {
    //       RGBAnimation = true;
    //       rgblight_mode(RGBLIGHT_MODE_RAINBOW_SWIRL + 1);
    //       RGB_current_mode = rgblight_config.mode;
    //     }
    //   #endif
    //   break;
    // case RGB3:
    //   #ifdef RGBLIGHT_ENABLE
    //     if (record->event.pressed) {
    //       RGBAnimation = true;
    //       rgblight_mode(RGBLIGHT_MODE_KNIGHT);
    //       RGB_current_mode = rgblight_config.mode;
    //     }
    //   #endif
    //   break;
    case MAC:
      if (record->event.pressed) {
        set_mac_mode(true);
      }
      break;
    case WIN:
      if (record->event.pressed) {
        set_mac_mode(false);
      }
      break;
    // default:
    //   // break;
    //   return false;
  }

  return true;
}

// //keyboard start-up code. Runs once when the firmware starts up.
// void matrix_init_user(void) {
//     #ifdef RGBLIGHT_ENABLE
//       RGB_current_mode = rgblight_config.mode;
//     #endif
// }

//assign the right code to your layers for OLED display
#define L_BASE 0
#define L_OPT 2
#define L_FUNC 4
#define L_SYM 8
#define L_NUM 16
#define L_FNLAYER 64
#define L_NUMLAY 128
#define L_NLOWER 136
#define L_NFNLAYER 192
#define L_MOUSECURSOR 256

// // LED Effect
// #ifdef RGBLIGHT_ENABLE
// // unsigned char rgb[7][5][3];
// unsigned char rgb[6][1][3];
// void led_ripple_effect(char r, char g, char b) {
//     static int scan_count = -10;
//     static int keys[] = { 6, 6, 6, 7, 7 };
//     static int keys_sum[] = { 0, 6, 12, 18, 25 };

//     if (scan_count == -1) {
//       rgblight_enable_noeeprom();
//       rgblight_mode(RGBLIGHT_MODE_STATIC_LIGHT);
//     } else if (scan_count >= 0 && scan_count < 5) {
//       for (unsigned char c=keybuf_begin; c!=keybuf_end; c++) {
//         int i = c;
//         // FIXME:

//         int y = scan_count;
//         int dist_y = abs(y - keybufs[i].row);
//         for (int x=0; x<keys[y]; x++) {
//           int dist = abs(x - keybufs[i].col) + dist_y;
//           if (dist <= keybufs[i].frame) {
//             int elevation = MAX(0, (8 + dist - keybufs[i].frame)) << 2;
//             if (elevation) {
//               if ((rgb[x][y][0] != 255) && r) { rgb[x][y][0] = MIN(255, elevation + rgb[x][y][0]); }
//               if ((rgb[x][y][1] != 255) && g) { rgb[x][y][1] = MIN(255, elevation + rgb[x][y][1]); }
//               if ((rgb[x][y][2] != 255) && b) { rgb[x][y][2] = MIN(255, elevation + rgb[x][y][2]); }
//             }
//           }
//         }
//       }
//     } else if (scan_count == 5) {
//       for (unsigned char c=keybuf_begin; c!=keybuf_end; c++) {
//         int i = c;
//         if (keybufs[i].frame < 18) {
//           keybufs[i].frame ++;
//         } else {
//           keybuf_begin ++;
//         }
//       }
//     } else if (scan_count >= 6 && scan_count <= 10) {
//       int y = scan_count - 6;
//       for (int x=0; x<keys[y]; x++) {
//         int at = keys_sum[y] + ((y & 1) ? x : (keys[y] - x - 1));
//         led[at].r = rgb[x][y][0];
//         led[at].g = rgb[x][y][1];
//         led[at].b = rgb[x][y][2];
//       }
//       rgblight_set();
//     } else if (scan_count == 11) {
//       memset(rgb, 0, sizeof(rgb));
//     }
//     scan_count++;
//     if (scan_count >= 12) { scan_count = 0; }
// }
// #endif

uint8_t layer_state_old;

//runs every scan cycle (a lot)
void matrix_scan_user(void) {
  if(delay_key_stat && (timer_elapsed(key_timer) > DELAY_TIME)){
    register_delay_code(_BASE);
    if(!delay_key_pressing){
      unregister_delay_code();
    }
  }

  if(layer_state_old != layer_state){
    switch (layer_state) {
      case L_BASE:
        break;
      case L_OPT:
        register_delay_code(_OPT);
        unregister_delay_code();
        break;
      case L_NUM:
        register_delay_code(_NUM);
        unregister_delay_code();
        break;
      case L_SYM:
        register_delay_code(_SYM);
        unregister_delay_code();
        break;
      case L_FUNC:
        register_delay_code(_FUNC);
        unregister_delay_code();
        break;
    }
    layer_state_old = layer_state;
  }

  // #ifdef RGBLIGHT_ENABLE
  //   if(!RGBAnimation){
  //     switch (layer_state) {
  //       case L_BASE:
  //           led_ripple_effect(0,112,127);
  //         break;
  //       case L_OPT:
  //           led_ripple_effect(127,0,100);
  //         break;
  //       case L_NUM:
  //           led_ripple_effect(127,23,0);
  //         break;
  //       case L_SYM:
  //           led_ripple_effect(0,127,0);
  //         break;
  //       case L_FUNC:
  //           led_ripple_effect(127,0,61);
  //         break;
  //       }
  //   }
  // #endif
}
