#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// 定义常量
#define MAP_ROWS 8
#define MAP_COLS 30
#define MAX_PLAYERS 4
#define MAX_PROPERTIES 10
#define MAX_ITEMS 10

// 玩家结构体
typedef struct {
    char name[20];
    char symbol;
    int money;
    int points;
    int position;
    int properties[MAX_PROPERTIES];
    int property_count;
    int items[MAX_ITEMS]; // 0:无 1:路障 2:机器娃娃 3:炸弹
    int item_count;
    int hospitalized;
    int imprisoned;
    int god_mode; // 财神附身
} Player;

// 地图格子类型
typedef struct {
    char type; // S:起点 O:空地 T:道具屋 G:礼品屋 M:魔法屋 H:医院 P:监狱 $:矿地
    int owner; // -1:无主, 0-3:玩家索引
    int level; // 0:空地, 1:茅屋, 2:洋房, 3:摩天楼
    int price;
    int toll; // 过路费
    int has_item; // 是否有道具
    int item_type; // 道具类型
} Cell;

// 游戏状态
 extern Cell map[MAP_ROWS][MAP_COLS];
 extern Player players[MAX_PLAYERS];
 extern int player_count;
 extern int current_player;
 extern int game_over;


// 函数声明
void init_map();
void init_players(int count, int initial_money);
void display_map();
void display_player_status(int player_index);
int roll_dice();
void move_player(int player_index, int steps);
void buy_property(int player_index);
void upgrade_property(int player_index);
void pay_toll(int player_index);
void handle_position(int player_index);
void use_block(int player_index, int distance);
void use_robot(int player_index);
void use_bomb(int player_index, int distance);
void game_loop();
void position_to_coord(int position, int *row, int *col);
void show_help();

