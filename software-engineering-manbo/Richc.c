#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <direct.h>
#include "cJSON.h"

// 函数声明
void init_map();

// 定义常量
#define MAP_ROWS 8
#define MAP_COLS 29
#define MAX_PLAYERS 4
#define MAX_PROPERTIES 10
#define MAX_ITEMS 10
#define TOTAL_CELLS (2 * (MAP_ROWS + MAP_COLS - 2)) // 边界上的总格子数

// 新数据结构定义
typedef struct{
    int bomb;
    int barrier;
    int robot;
    int total;
}Item;

typedef struct{
    int god;
    int prison;
    int hospitail;
}Buff;

// 玩家结构体
typedef struct {
    int index;
    char name[20];
    char symbol;
    int fund;
    int credit; 
    int location;
    bool alive;
    Item *items;
    Buff *buff;
} Player;

typedef struct{
    int now_player;
    int next_player;
    bool started;
    bool ended;
    int winner;
    bool quit_early;
} Game;

typedef struct{
    int bomb[TOTAL_CELLS];
    int barrier[TOTAL_CELLS];
} Prop;

// 房屋/领地结构体
typedef struct {
    char owner; // 改为字符类型，存储玩家代号
    int level;
} House;

// 地图格子类型
typedef struct {
    char type; // S:起点 O:空地 T:道具屋 G:礼品屋 M:魔法屋 H:医院 P:监狱 $:矿地
    int price;
    int toll; // 过路费
    Prop *placed_prop;
    House* houses; // 指向对应的房屋信息
} Cell;

// 游戏状态
Cell map[MAP_ROWS][MAP_COLS];
House houses[TOTAL_CELLS]; // 所有房屋信息
Player players[MAX_PLAYERS];
Prop placed_props;
Item items[MAX_PLAYERS];
Buff buffs[MAX_PLAYERS];
Game game_state;

int player_count;
char save_path[100];

const char* get_player_name_from_symbol(char symbol) {
    switch (symbol) {
        case 'Q': return "钱夫人";
        case 'A': return "阿土伯";
        case 'S': return "孙小美";
        case 'J': return "金贝贝";
        default: return "未知玩家";
    }
}

void dump_json(int player_count) {
    char str[2];
   cJSON *root = cJSON_CreateObject();

    // players
    cJSON *players_arr = cJSON_CreateArray();
    for (int i = 0; i < player_count; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "index", players[i].index);
        str[0] = players[i].symbol;
        str[1] = '\0';
        cJSON_AddStringToObject(p, "name", str);
        cJSON_AddNumberToObject(p, "fund", players[i].fund);
        cJSON_AddNumberToObject(p, "credit", players[i].credit);
        cJSON_AddNumberToObject(p, "location", players[i].location);
        cJSON_AddBoolToObject(p, "alive", players[i].alive);

        // prop
        cJSON *prop = cJSON_CreateObject();
        cJSON_AddNumberToObject(prop, "bomb", players[i].items->bomb);
        cJSON_AddNumberToObject(prop, "barrier", players[i].items->barrier);
        cJSON_AddNumberToObject(prop, "robot", players[i].items->robot);
        cJSON_AddNumberToObject(prop, "total", players[i].items->total);
        cJSON_AddItemToObject(p, "prop", prop);

        // buff
        cJSON *buff = cJSON_CreateObject();
        cJSON_AddNumberToObject(buff, "god", players[i].buff->god);
        cJSON_AddNumberToObject(buff, "prison", players[i].buff->prison);
        cJSON_AddNumberToObject(buff, "hospital", players[i].buff->hospitail);
        cJSON_AddItemToObject(p, "buff", buff);

        cJSON_AddItemToArray(players_arr, p);
    }
    cJSON_AddItemToObject(root, "players", players_arr);

    // houses
    cJSON *houses_obj = cJSON_CreateObject();
    for (int i = 0; i < TOTAL_CELLS; i++) {
        cJSON *h = cJSON_CreateObject();
        
        // 修改这里：将字符串改为单个字符
        char owner_char[2] = {houses[i].owner, '\0'};
        cJSON_AddStringToObject(h, "owner", owner_char);
        
        cJSON_AddNumberToObject(h, "level", houses[i].level);

        char idx[16];
        sprintf(idx, "%d", i);
        cJSON_AddItemToObject(houses_obj, idx, h);
    }
    cJSON_AddItemToObject(root, "houses", houses_obj);

    // placed_prop
    cJSON *placed_prop = cJSON_CreateObject();
    cJSON *bomb_arr = cJSON_CreateArray();
    cJSON *barrier_arr = cJSON_CreateArray();
    for (int i = 0; i < TOTAL_CELLS; i++) {
        if (placed_props.bomb[i] > 0)
            cJSON_AddItemToArray(bomb_arr, cJSON_CreateNumber(i));
        if (placed_props.barrier[i] > 0)
            cJSON_AddItemToArray(barrier_arr, cJSON_CreateNumber(i));
    }
    cJSON_AddItemToObject(placed_prop, "bomb", bomb_arr);
    cJSON_AddItemToObject(placed_prop, "barrier", barrier_arr);
    cJSON_AddItemToObject(root, "placed_prop", placed_prop);

    // game
    cJSON *game_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(game_obj, "now_player", game_state.now_player);
    cJSON_AddNumberToObject(game_obj, "next_player", game_state.next_player);
    cJSON_AddBoolToObject(game_obj, "started", game_state.started);
    cJSON_AddBoolToObject(game_obj, "ended", game_state.ended);
    cJSON_AddNumberToObject(game_obj, "winner", game_state.winner);
    cJSON_AddItemToObject(root, "game", game_obj);

    // 输出文件
    char *json_str = cJSON_Print(root);
    FILE *fp = fopen(save_path, "w");
    if (fp) {
        fputs(json_str, fp);
        fclose(fp);
    }
    free(json_str);
    cJSON_Delete(root);
}

bool import_json(const char *filename) {
    // 先初始化地图
    init_map();

    // 初始化玩家的items和buff指针
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].items = &items[i];
        players[i].buff = &buffs[i];
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char*)malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) {
        return false;
    }

    // 解析 players 数组
    cJSON *players_json = cJSON_GetObjectItem(root, "players");
    int num_players = 0; // 初始化玩家数量
    if (players_json && cJSON_IsArray(players_json)) {
        num_players = cJSON_GetArraySize(players_json);
        for (int i = 0; i < num_players && i < MAX_PLAYERS; i++) {
            // 解析每个玩家数据
            cJSON *player_json = cJSON_GetArrayItem(players_json, i);
            if (!player_json) continue;

            cJSON *index_json = cJSON_GetObjectItem(player_json, "index");
            if (index_json && cJSON_IsNumber(index_json)) {
                int index = index_json->valueint;
                if (index < 0 || index >= MAX_PLAYERS) continue;

                players[index].index = index;

                cJSON *name_json = cJSON_GetObjectItem(player_json, "name");
                if (name_json && cJSON_IsString(name_json)) {
                    // 修复：将name字段的值作为symbol，而不是name
                    players[index].symbol = name_json->valuestring[0];
                    
                    // 根据symbol设置正确的name
                    switch (players[index].symbol) {
                        case 'Q':
                            strcpy(players[index].name, "钱夫人");
                            break;
                        case 'A':
                            strcpy(players[index].name, "阿土伯");
                            break;
                        case 'S':
                            strcpy(players[index].name, "孙小美");
                            break;
                        case 'J':
                            strcpy(players[index].name, "金贝贝");
                            break;
                        default:
                            // 如果symbol不是预期的值，保留原始值
                            break;
                    }
                }

                cJSON *fund_json = cJSON_GetObjectItem(player_json, "fund");
                if (fund_json && cJSON_IsNumber(fund_json)) {
                    players[index].fund = fund_json->valueint;
                }

                cJSON *credit_json = cJSON_GetObjectItem(player_json, "credit");
                if (credit_json && cJSON_IsNumber(credit_json)) {
                    players[index].credit = credit_json->valueint;
                }

                cJSON *location_json = cJSON_GetObjectItem(player_json, "location");
                if (location_json && cJSON_IsNumber(location_json)) {
                    players[index].location = location_json->valueint;
                }

                cJSON *alive_json = cJSON_GetObjectItem(player_json, "alive");
                if (alive_json && cJSON_IsBool(alive_json)) {
                    players[index].alive = cJSON_IsTrue(alive_json);
                }

                // 解析 prop
                cJSON *prop_json = cJSON_GetObjectItem(player_json, "prop");
                if (prop_json) {
                    cJSON *bomb_json = cJSON_GetObjectItem(prop_json, "bomb");
                    if (bomb_json && cJSON_IsNumber(bomb_json)) {
                        players[index].items->bomb = bomb_json->valueint;
                    }
                    cJSON *barrier_json = cJSON_GetObjectItem(prop_json, "barrier");
                    if (barrier_json && cJSON_IsNumber(barrier_json)) {
                        players[index].items->barrier = barrier_json->valueint;
                    }
                    cJSON *robot_json = cJSON_GetObjectItem(prop_json, "robot");
                    if (robot_json && cJSON_IsNumber(robot_json)) {
                        players[index].items->robot = robot_json->valueint;
                    }
                    cJSON *total_json = cJSON_GetObjectItem(prop_json, "total");
                    if (total_json && cJSON_IsNumber(total_json)) {
                        players[index].items->total = total_json->valueint;
                    }
                }

                // 解析 buff
                cJSON *buff_json = cJSON_GetObjectItem(player_json, "buff");
                if (buff_json) {
                    cJSON *god_json = cJSON_GetObjectItem(buff_json, "god");
                    if (god_json && cJSON_IsNumber(god_json)) {
                        players[index].buff->god = god_json->valueint;
                    }
                    cJSON *prison_json = cJSON_GetObjectItem(buff_json, "prison");
                    if (prison_json && cJSON_IsNumber(prison_json)) {
                        players[index].buff->prison = prison_json->valueint;
                    }
                    cJSON *hospital_json = cJSON_GetObjectItem(buff_json, "hospital");
                    if (hospital_json && cJSON_IsNumber(hospital_json)) {
                        players[index].buff->hospitail = hospital_json->valueint;
                    }
                }
                player_count = num_players;
            }
        }
    }

    // 解析 houses
    cJSON *houses_json = cJSON_GetObjectItem(root, "houses");
    if (houses_json && cJSON_IsObject(houses_json)) {
        cJSON *house_item = NULL;
        cJSON_ArrayForEach(house_item, houses_json) {
            if (house_item && cJSON_IsObject(house_item)) {
                int house_index = atoi(house_item->string);
                if (house_index < 0 || house_index >= TOTAL_CELLS) continue;

                cJSON *owner_json = cJSON_GetObjectItem(house_item, "owner");
                if (owner_json && cJSON_IsString(owner_json)) {
                    // 修改这里：只取第一个字符
                    houses[house_index].owner = owner_json->valuestring[0];
                }

                cJSON *level_json = cJSON_GetObjectItem(house_item, "level");
                if (level_json && cJSON_IsNumber(level_json)) {
                    houses[house_index].level = level_json->valueint;
                }
            }
        }
    }

    // 解析 placed_prop
    cJSON *placed_prop_json = cJSON_GetObjectItem(root, "placed_prop");
    if (placed_prop_json) {
        // 初始化 placed_props
        memset(&placed_props, 0, sizeof(Prop));

        cJSON *bomb_json = cJSON_GetObjectItem(placed_prop_json, "bomb");
        if (bomb_json && cJSON_IsArray(bomb_json)) {
            int num_bombs = cJSON_GetArraySize(bomb_json);
            for (int i = 0; i < num_bombs; i++) {
                cJSON *bomb_item = cJSON_GetArrayItem(bomb_json, i);
                if (bomb_item && cJSON_IsNumber(bomb_item)) {
                    int pos = bomb_item->valueint;
                    if (pos >= 0 && pos < TOTAL_CELLS) {
                        placed_props.bomb[pos] = 1;
                    }
                }
            }
        }

        cJSON *barrier_json = cJSON_GetObjectItem(placed_prop_json, "barrier");
        if (barrier_json && cJSON_IsArray(barrier_json)) {
            int num_barriers = cJSON_GetArraySize(barrier_json);
            for (int i = 0; i < num_barriers; i++) {
                cJSON *barrier_item = cJSON_GetArrayItem(barrier_json, i);
                if (barrier_item && cJSON_IsNumber(barrier_item)) {
                    int pos = barrier_item->valueint;
                    if (pos >= 0 && pos < TOTAL_CELLS) {
                        placed_props.barrier[pos] = 1;
                    }
                }
            }
        }
    }

    // 解析 game
    cJSON *game_json = cJSON_GetObjectItem(root, "game");
    if (game_json) {
        cJSON *now_player_json = cJSON_GetObjectItem(game_json, "now_player");
        if (now_player_json && cJSON_IsNumber(now_player_json)) {
            game_state.now_player = now_player_json->valueint;
        }

        cJSON *next_player_json = cJSON_GetObjectItem(game_json, "next_player");
        if (next_player_json && cJSON_IsNumber(next_player_json)) {
            game_state.next_player = next_player_json->valueint;
        }

        cJSON *started_json = cJSON_GetObjectItem(game_json, "started");
        if (started_json && cJSON_IsBool(started_json)) {
            game_state.started = cJSON_IsTrue(started_json);
        }

        cJSON *ended_json = cJSON_GetObjectItem(game_json, "ended");
        if (ended_json && cJSON_IsBool(ended_json)) {
            game_state.ended = cJSON_IsTrue(ended_json);
        }

        cJSON *winner_json = cJSON_GetObjectItem(game_json, "winner");
        if (winner_json && cJSON_IsNumber(winner_json)) {
            game_state.winner = winner_json->valueint;
        }
    }

    cJSON_Delete(root);
    return true;
}

void make_dirs(char *path) {
    char tmp[512];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);

    if (tmp[len - 1] == '/' || tmp[len - 1] == '\\') {
        tmp[len - 1] = '\0';  // 去掉末尾的 / 或 
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
            _mkdir(tmp);   // 尝试创建子目录（已存在会失败，但不影响）
            *p = '/';     // 恢复分隔符
        }
    }
    _mkdir(tmp);  // 创建最后一级
}

// 初始化地图为方形边界
void init_map() {
    // 初始化所有格子为' '
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            map[i][j].type = ' ';
            map[i][j].price = 0;
            map[i][j].toll = 0;
            map[i][j].houses = NULL;
            map[i][j].placed_prop = &placed_props;
        }
    }
    
    // 初始化道具数组
    memset(&placed_props, 0, sizeof(Prop));
    
    // 初始化所有房屋
    for (int i = 0; i < TOTAL_CELLS; i++) {
        houses[i].owner = '-'; // 使用 '-' 表示无主
        houses[i].level = 0;
    }
    
    // 设置边界路径
    // 顶部行
    for (int j = 0; j < MAP_COLS; j++) {
        map[0][j].type = 'O';
        map[0][j].price = 200;
        map[0][j].toll = 100;
    }
    
    // 底部行
    for (int j = 0; j < MAP_COLS; j++) {
        map[MAP_ROWS-1][j].type = 'O';
        map[MAP_ROWS-1][j].price = 300;
        map[MAP_ROWS-1][j].toll = 150;
    }
    
    // 左侧列
    for (int i = 1; i < MAP_ROWS-1; i++) {
        map[i][0].type = 'O';
        map[i][0].price = 200;
        map[i][0].toll = 100;
    }
    
    // 右侧列
    for (int i = 1; i < MAP_ROWS-1; i++) {
        map[i][MAP_COLS-1].type = 'O';
        map[i][MAP_COLS-1].price = 500;
        map[i][MAP_COLS-1].toll = 250;
    }
    
    // 设置特殊地点 - 根据新的列数调整位置
    map[0][0].type = 'S'; // 起点
    map[0][14].type = 'H'; // 医院
    map[0][MAP_COLS-1].type = 'T'; // 道具屋
    
    map[MAP_ROWS-1][0].type = 'M'; // 魔法屋
    map[MAP_ROWS-1][14].type = 'P'; // 监狱
    map[MAP_ROWS-1][MAP_COLS-1].type = 'G'; // 礼品屋
    
    // 矿地位置不变
    map[1][0].type = '$';
    map[2][0].type = '$';
    map[3][0].type = '$';
    map[4][0].type = '$';
    map[5][0].type = '$';
    map[6][0].type = '$';

    // 将边界上的房屋指针指向对应的houses元素
    int pos = 0;

    // 顶部行（从左到右）
    for (int j = 0; j < MAP_COLS; j++) {
        map[0][j].houses = &houses[pos++];
    }

    // 右侧列（从上到下，跳过顶部和底部）
    for (int i = 1; i < MAP_ROWS-1; i++) {
        map[i][MAP_COLS-1].houses = &houses[pos++];
    }

    // 底部行（从右到左）
    for (int j = MAP_COLS-1; j >= 0; j--) {
        map[MAP_ROWS-1][j].houses = &houses[pos++];
    }

    // 左侧列（从下到上，跳过顶部和底部）
    for (int i = MAP_ROWS-2; i > 0; i--) {
        map[i][0].houses = &houses[pos++];
    }

    // 验证我们使用了正确数量的houses
    if (pos != TOTAL_CELLS) {
        printf("警告: houses指针设置可能有问题，使用了%d个houses，但TOTAL_CELLS=%d\n", pos, TOTAL_CELLS);
    }
}

// 将一维位置转换为二维坐标
void position_to_coord(int position, int *row, int *col) {
    int total_cells = TOTAL_CELLS;
    
    position = position % total_cells;
    
    if (position < MAP_COLS) {
        // 顶部行（从左到右）
        *row = 0;
        *col = position;
    } else if (position < MAP_COLS + (MAP_ROWS - 2)) {
        // 右侧列（从上到下）
        *row = position - MAP_COLS + 1;
        *col = MAP_COLS - 1;
    } else if (position < MAP_COLS + (MAP_ROWS - 2) + MAP_COLS) {
        // 底部行（从右到左）
        *row = MAP_ROWS - 1;
        *col = MAP_COLS - 1 - (position - (MAP_COLS + (MAP_ROWS - 2)));
    } else {
        // 左侧列（从下到上）
        int left_start = MAP_COLS + (MAP_ROWS - 2) + MAP_COLS;
        int pos_in_left = position - left_start;
        *row = MAP_ROWS - 2 - pos_in_left;
        *col = 0;
    }
}

// 将二维坐标转换为一维位置
int coord_to_position(int row, int col) {
    if (row == 0) {
        return col; // 顶部行
    } else if (col == MAP_COLS - 1) {
        return MAP_COLS + (row - 1); // 右侧列，行从1到6
    } else if (row == MAP_ROWS - 1) {
        return MAP_COLS + (MAP_ROWS - 2) + (MAP_COLS - 1 - col); // 底部行
    } else if (col == 0) {
        return MAP_COLS + (MAP_ROWS - 2) + MAP_COLS + (MAP_ROWS - 2 - row); // 左侧列
    }
    return -1; // 无效坐标
}

// 检查指定位置是否有道具
int has_prop_at_position(int position, int prop_type) {
    switch (prop_type) {
        case 1: // 路障
            return placed_props.barrier[position] > 0;
        case 2: // 机器娃娃
            // 机器娃娃不在地图上显示，直接使用
            return 0;
        case 3: // 炸弹
            return placed_props.bomb[position] > 0;
        default:
            return 0;
    }
}

// 初始化玩家
void init_players(char* player_chars, int initial_fund) {
    player_count = strlen(player_chars);
    game_state.quit_early = false;
    // 初始化游戏状态
    game_state.now_player = 0;
    game_state.next_player = 1;
    game_state.started = true;
    game_state.ended = false;
    game_state.winner = -1;
    
    // 初始化玩家属性
    for (int i = 0; i < player_count; i++) {
        players[i].index = i;
        players[i].fund = initial_fund;
        players[i].credit = 500;
        players[i].location = 0;
        players[i].alive = true;
        
        // 初始化道具
        items[i].bomb = 0;
        items[i].barrier = 0;
        items[i].robot = 0;
        items[i].total = 0;
        players[i].items = &items[i];
        
        // 初始化状态
        buffs[i].god = 0;
        buffs[i].prison = 0;
        buffs[i].hospitail = 0;
        players[i].buff = &buffs[i];
        
        // 根据输入的字符设置玩家名称和符号
        switch (player_chars[i]) {
            case '1':
                strcpy(players[i].name, "钱夫人");
                players[i].symbol = 'Q';
                break;
            case '2':
                strcpy(players[i].name, "阿土伯");
                players[i].symbol = 'A';
                break;
            case '3':
                strcpy(players[i].name, "孙小美");
                players[i].symbol = 'X';
                break;
            case '4':
                strcpy(players[i].name, "金贝贝");
                players[i].symbol = 'J';
                break;
        }
    }
}

// 显示地图
void display_map() {
    printf("\n当前地图状态:\n");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            // 检查是否有玩家在此位置
            int player_here = -1;
            for (int k = 0; k < MAX_PLAYERS; k++) {
                if (!players[k].alive) continue;
                
                int row, col;
                position_to_coord(players[k].location, &row, &col);
                if (row == i && col == j) {
                    player_here = k;
                    break;
                }
            }
            
            if (player_here != -1) {
                if(players[player_here].symbol == 'Q')
                    printf("\033[1;31m%c\033[0m", players[player_here].symbol);
                else if(players[player_here].symbol == 'A')
                    printf("\033[1;32m%c\033[0m", players[player_here].symbol);
                else if(players[player_here].symbol == 'S')
                    printf("\033[1;34m%c\033[0m", players[player_here].symbol); 
                else if(players[player_here].symbol == 'J')
                    printf("\033[1;33m%c\033[0m", players[player_here].symbol); 
            } else {
                // 检查是否有道具
                int position = coord_to_position(i, j);
                if (position != -1) {
                    if (placed_props.barrier[position] > 0) {
                        printf("#"); // 路障
                    } else if (placed_props.bomb[position] > 0) {
                        printf("@"); // 炸弹
                    } else {
                        switch (map[i][j].type) {
                            case 'S': printf("S"); break;
                            case 'O': 
                                if (map[i][j].houses != NULL && map[i][j].houses->owner != '-') {
                                    char owner_symbol = map[i][j].houses->owner;
                                    // 根据 owner_symbol 显示颜色
                                    if (owner_symbol == 'Q')
                                        printf("\033[1;31m%c\033[0m", map[i][j].houses->level + '0');
                                    else if (owner_symbol == 'A')
                                        printf("\033[1;32m%c\033[0m", map[i][j].houses->level + '0');
                                    else if (owner_symbol == 'S')
                                        printf("\033[1;34m%c\033[0m", map[i][j].houses->level + '0');
                                    else if (owner_symbol == 'J')
                                        printf("\033[1;33m%c\033[0m", map[i][j].houses->level + '0');
                                } else {
                                    printf("O");
                                }
                                break;
                            case 'T': printf("T"); break;
                            case 'G': printf("G"); break;
                            case 'M': printf("M"); break;
                            case 'H': printf("H"); break;
                            case 'P': printf("P"); break;
                            default: printf("%c", map[i][j].type);
                        }
                    }
                } else {
                    printf(" ");
                }
            }
        }
        printf("\n");
    }
    
    printf("------------------------------------------------------------\n");
    printf("图例: S-起点 O-空地 T-道具屋 G-礼品屋 M-魔法屋 $-矿地 H-医院 P-监狱\n");
    printf("      数字-地产等级(0-3) Q-钱夫人 A-阿土伯 S-孙小美 J-金贝贝\n");
    printf("      #-路障 @-炸弹\n");
}

// 显示玩家状态
void display_player_status(int player_index) {
    if (!players[player_index].alive) return;
    
    printf("\n%s 的状态:\n", players[player_index].name);
    printf("资金: %d元\n", players[player_index].fund);
    printf("点数: %d点\n", players[player_index].credit);
    
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    printf("位置: (%d, %d)\n", row, col);
    
    printf("道具: 路障:%d个, 炸弹:%d个, 机器娃娃:%d个\n", 
           players[player_index].items->barrier, 
           players[player_index].items->bomb,
           players[player_index].items->robot);
    
    if (players[player_index].buff->hospitail > 0) {
        printf("状态: 住院中 (%d回合后出院)\n", players[player_index].buff->hospitail);
    } else if (players[player_index].buff->prison > 0) {
        printf("状态: 监禁中 (%d回合后释放)\n", players[player_index].buff->prison);
    } else if (players[player_index].buff->god > 0) {
        printf("状态: 财神附身 (%d回合有效)\n", players[player_index].buff->god);
    } else {
        printf("状态: 正常\n");
    }
}

// 掷骰子
int roll_dice() {
    return rand() % 6 + 1;
}

// 移动玩家
void move_player(int player_index, int steps) {
    players[player_index].location = (players[player_index].location + steps) % TOTAL_CELLS;
    
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    printf("%s 移动了 %d 步，到达位置 (%d, %d)\n", 
           players[player_index].name, steps, row, col);
}

// 购买地产
void buy_property(int player_index) {
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    
    if (map[row][col].type != 'O') {
        printf("此处不能购买地产\n");
        return;
    }
    
    if (map[row][col].houses == NULL || map[row][col].houses->owner != '-') {
        printf("此地已有主人\n");
        return;
    }
    
    if (players[player_index].fund >= map[row][col].price) {
        players[player_index].fund -= map[row][col].price;
        map[row][col].houses->owner = players[player_index].symbol; // 直接赋值符号
        map[row][col].houses->level = 0;
        
        printf("%s 购买了位置 (%d, %d) 的地产，花费 %d元\n", 
               players[player_index].name, row, col, map[row][col].price);
    } else {
        printf("资金不足，无法购买此地产\n");
    }
}

// 升级地产
void upgrade_property(int player_index) {
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    
    if (map[row][col].houses == NULL || map[row][col].houses->owner != players[player_index].symbol) {
        printf("这不是你的地产\n");
        return;
    }
    
    if (map[row][col].houses->level < 3 && players[player_index].fund >= map[row][col].price) {
        players[player_index].fund -= map[row][col].price;
        map[row][col].houses->level++;
        printf("%s 升级了位置 (%d, %d) 的地产，现在是 %d 级，花费 %d元\n", 
               players[player_index].name, row, col, map[row][col].houses->level, map[row][col].price);
    } else {
        printf("无法升级此地产\n");
    }
}

// 卖地产函数
void sell_property(int player_index, int position) {
    if (position < 0 || position >= TOTAL_CELLS) {
        printf("无效的地产位置\n");
        return;
    }
    
    // 将一维位置转换为二维坐标
    int row, col;
    position_to_coord(position, &row, &col);
    
    // 检查该位置是否有房屋
    if (map[row][col].houses == NULL) {
        printf("该位置没有地产\n");
        return;
    }
    
    // 检查该地产是否属于当前玩家
    if (map[row][col].houses->owner != players[player_index].symbol) {
        printf("这不是你的地产\n");
        return;
    }
    
    // 计算总投资额（购买价格 + 所有升级费用）
    int total_investment = map[row][col].price * (map[row][col].houses->level + 1);
    
    // 玩家获得总投资的两倍
    int sale_price = total_investment * 2;
    players[player_index].fund += sale_price;
    
    // 重置地产
    map[row][col].houses->owner = '-';
    map[row][col].houses->level = 0;
    
    printf("%s 卖掉了位置 %d 的地产，获得 %d元\n", 
           players[player_index].name, position, sale_price);
}

// 支付过路费
void pay_toll(int player_index) {
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    
    // 添加坐标有效性检查
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS) {
        printf("错误：无效的坐标 (%d, %d)\n", row, col);
        return;
    }
    
    if (map[row][col].houses == NULL) {
        printf("此处无需支付过路费\n");
        return;
    }
    
    // 检查houses指针是否有效
    if (map[row][col].houses < &houses[0] || map[row][col].houses > &houses[TOTAL_CELLS-1]) {
        printf("错误：无效的houses指针\n");
        return;
    }
    
    // 找到地主
    int owner_index = -1;
    char owner_symbol = map[row][col].houses->owner;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].alive) continue;
        if (players[i].symbol == owner_symbol) {
            owner_index = i;
            break;
        }
    }
    
    if (owner_index == -1) {
        printf("错误：找不到地主信息，owner=%c\n", owner_symbol);
        return;
    }
    
    // 修改：计算投资总金额的一半作为过路费
    int total_investment = map[row][col].price * (map[row][col].houses->level + 1);
    int toll = total_investment / 2;
    
    if (players[player_index].buff->god > 0) {
        printf("财神附身，免付过路费\n");
        return;
    }
    
    if (players[owner_index].buff->hospitail > 0 || players[owner_index].buff->prison > 0) {
        printf("地主正在医院或监狱中，免付过路费\n");
        return;
    }
    
    if (players[player_index].fund >= toll) {
        players[player_index].fund -= toll;
        players[owner_index].fund += toll;
        printf("%s 向 %s 支付了过路费 %d元 (投资总额 %d元的一半)\n", 
               players[player_index].name, players[owner_index].name, toll, total_investment);
    } else {
        printf("%s 资金不足，无法支付过路费，破产了！\n", players[player_index].name);
        players[player_index].alive = false;
        game_state.ended = true; // 简化处理，实际应该检查是否只剩一个玩家
    }
}

// 处理玩家到达的位置
void handle_position(int player_index) {
    int row, col;
    position_to_coord(players[player_index].location, &row, &col);
    Cell *cell = &map[row][col];
    
    printf("%s 到达了位置 (%d, %d): ", players[player_index].name, row, col);
    
    // 先检查是否有道具效果
    int position = coord_to_position(row, col);
    if (position != -1) {
        if (placed_props.bomb[position] > 0) {
            printf("被炸弹炸伤，住院3天\n");
            players[player_index].buff->hospitail = 3;
            placed_props.bomb[position] = 0; // 移除炸弹
            return;
        } else if (placed_props.barrier[position] > 0) {
            printf("被路障拦截，停止一回合\n");
            return;
        }
    }
    
    switch (cell->type) {
        case 'S':
            printf("起点\n");
            break;
            
        case 'O':
            if (cell->houses == NULL || cell->houses->owner == '-') {
                printf("空地，可以购买\n");
                printf("购买价格: %d元\n", cell->price);
                char command[10];
                while(1){
                    printf("是否购买地产?(y/n): ");
                    scanf("%s", command);
                    if (strcmp(command, "y") == 0){
                        buy_property(player_index);
                        break;
                    }
                    else if(strcmp(command, "n") == 0)
                    break;
                    else
                    printf("非法输入\n");
                }
            } else if (cell->houses->owner == players[player_index].symbol) {
                printf("自己的地产，可以升级\n");
                printf("升级价格: %d元\n", cell->price);
                char command[10];
                while(1){
                    printf("是否升级地产?(y/n): ");
                    scanf("%s", command);
                    if (strcmp(command, "y") == 0) {
                        upgrade_property(player_index);
                        break;
                    }
                    else if(strcmp(command, "n") == 0)
                    break;
                    else
                    printf("非法输入\n");
                }
            } else {
                const char* owner_name = get_player_name_from_symbol(cell->houses->owner);
                printf("%s 的地产，需要支付过路费\n", owner_name);
                pay_toll(player_index);
            }
            break;
            
        case 'T':
            printf("道具屋\n价格如下所示：\n");
            printf("1. 路障 50点数\n");
            printf("2. 机器娃娃 30点数\n");
            printf("3. 炸弹 50点数\n");
            printf("F. 退出道具屋\n");
        
        // 道具屋购买循环
        while (1) {
            printf("请输入道具编号购买道具 (1/2/3) 或 F 退出: ");
            char input[10];
            scanf("%s", input);
            
            // 清空输入缓冲区
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (strlen(input) == 1) {
                char choice = input[0];
                
                if (choice == 'F' || choice == 'f') {
                    printf("退出道具屋\n");
                    break;
                }
                
                int item;
                int cost = 0;
                
                switch (choice) {
                    case '1': 
                        item = 1;
                        cost = 50;
                        break;
                    case '2':
                        item = 2;
                        cost = 30;
                        break;
                    case '3':
                        item = 3;
                        cost = 50;
                        break;
                    default:
                        printf("无效的道具编号\n");
                        continue;
                }
                
                if (players[player_index].credit >= cost) {
                    players[player_index].credit -= cost;
                    switch (item) {
                        case 1: 
                            players[player_index].items->barrier++;
                            players[player_index].items->total++;
                            printf("获得了路障，当前路障数量: %d\n", players[player_index].items->barrier);
                            break;
                        case 2: 
                            players[player_index].items->robot++;
                            players[player_index].items->total++;
                            printf("获得了机器娃娃，当前机器娃娃数量: %d\n", players[player_index].items->robot);
                            break;
                        case 3: 
                            players[player_index].items->bomb++;
                            players[player_index].items->total++;
                            printf("获得了炸弹，当前炸弹数量: %d\n", players[player_index].items->bomb);
                            break;
                    }
                } else {
                    printf("点数不足，无法购买道具\n");
                }
            } else {
                printf("请输入单个字符\n");
            }
        }
        break;
            
        case 'G':
            printf("礼品屋\n");
            int gift = rand() % 3 + 1;
            switch (gift) {
                case 1:
                    players[player_index].fund += 2000;
                    printf("获得了 2000元奖金\n");
                    break;
                case 2:
                    players[player_index].credit += 200;
                    printf("获得了 200点\n");
                    break;
                case 3:
                    players[player_index].buff->god = 5;
                    printf("获得了财神附身，5回合有效\n");
                    break;
            }
            break;
            
        case 'M':
            printf("魔法屋\n");
            int effect = rand() % 3;
            switch (effect) {
                case 0:
                    players[player_index].fund += 1000;
                    printf("获得了 1000元\n");
                    break;
                case 1:
                    players[player_index].credit += 100;
                    printf("获得了 100点\n");
                    break;
                case 2:
                    players[player_index].fund -= 500;
                    printf("失去了 500元\n");
                    break;
            }
            break;

        case '$':
            printf("矿地\n");
            int points = 0;
            // 根据矿地的位置确定点数
            int position = coord_to_position(row, col);
            switch (position) {
                case 69: points = 20; break;
                case 68: points = 80; break;
                case 67: points = 100; break;
                case 66: points = 40; break;
                case 65: points = 80; break;
                case 64: points = 60; break;
                default: points = 0; break;
            }
            players[player_index].credit += points;
            printf("获得了 %d 点\n", points);
            break;
            
        case 'H':
            printf("医院\n");
            if (players[player_index].buff->hospitail == 0) {
                printf("只是路过医院\n");
            } else {
                printf("正在医院接受治疗\n");
            }
            break;
            
        case 'P':
            printf("监狱\n");
            if (players[player_index].buff->prison == 0) {
                // 玩家第一次到达监狱，设置监禁状态
                printf("被关进监狱，将禁闭2回合\n");
                players[player_index].buff->prison = 2;
            } 
            break;
            
        default:
            printf("未知地点\n");
            break;
    }
}

// 使用路障
void use_block(int player_index, int distance) {
    if (players[player_index].items->barrier > 0) {
        players[player_index].items->barrier--;
        players[player_index].items->total--;
        
        int target_pos = (players[player_index].location + distance + TOTAL_CELLS) % TOTAL_CELLS;
        placed_props.barrier[target_pos] = 1;
        
        int row, col;
        position_to_coord(target_pos, &row, &col);
        printf("在位置 (%d, %d) 放置了路障\n", row, col);
    } else {
        printf("没有路障道具\n");
    }
}

// 使用炸弹
void use_bomb(int player_index, int distance) {
    if (players[player_index].items->bomb > 0) {
        players[player_index].items->bomb--;
        players[player_index].items->total--;
        
        int target_pos = (players[player_index].location + distance + TOTAL_CELLS) % TOTAL_CELLS;
        placed_props.bomb[target_pos] = 1;
        
        int row, col;
        position_to_coord(target_pos, &row, &col);
        printf("在位置 (%d, %d) 放置了炸弹\n", row, col);
    } else {
        printf("没有炸弹道具\n");
    }
}

// 使用机器娃娃
void use_doll(int player_index, int distance) {
    if (players[player_index].items->robot > 0) {
        players[player_index].items->robot--;
        players[player_index].items->total--;
        
        // 机器娃娃的效果：清除前方10步内的所有道具
        for (int i = 1; i <= 10; i++) {
            int target_pos = (players[player_index].location + i) % TOTAL_CELLS;
            placed_props.barrier[target_pos] = 0;
            placed_props.bomb[target_pos] = 0;
        }
        
        printf("使用了机器娃娃，清除了前方10步内的所有道具\n");
    } else {
        printf("没有机器娃娃道具\n");
    }
}

// 显示帮助信息
void show_help() {
    printf("\n可用命令:\n");
    printf("  roll        - 掷骰子前进\n");
    printf("  robot       - 使用机器娃娃\n");
    printf("  bomb n      - 在当前位置前/后方n格放置炸弹\n");
    printf("  block n     - 在当前位置前/后方n格放置路障\n");
    printf("  query       - 查询自己当前资产\n");
    printf("  help        - 查看可输入指令帮助\n");
    printf("  1/2/3       - 在道具屋购买道具 (1:路障 2:机器娃娃 3:炸弹)\n");
    printf("  y/n         - 在购买/不买地产\n");
    printf("  y/n         - 在自己的地产上升级/不升级\n");
    printf("  quit        - 退出游戏并结算资产最多的玩家为胜者\n");
}

// 处理命令
int process_command(int player_index, char* command) {

    if (command[0] == '\0') {
        return 0; // 空命令，回合继续
    }

    if (strcmp(command, "roll") == 0) {
        int dice = roll_dice();
        printf("掷出了 %d 点\n", dice);
        move_player(player_index, dice);
        handle_position(player_index);
        return 1; // 回合结束
    }
    else if (strcmp(command, "query") == 0) {
        display_player_status(player_index);
        return 0; // 回合继续
    }
    else if (strcmp(command, "help") == 0) {
        show_help();
        return 0; // 回合继续
    }
    else if (strncmp(command, "bomb", 4) == 0) {
        int distance;
        if (sscanf(command + 5, "%d", &distance) == 1) {
            if (distance >= -10 && distance <= 10) {
                use_bomb(player_index, distance);
            } else {
                printf("距离必须在 -10 到 10 之间\n");
            }
        } else {
            printf("用法: bomb n (n为距离)\n");
        }
        return 0; // 回合继续
    }
    else if (strncmp(command, "block", 5) == 0) {
        int distance;
        if (sscanf(command + 6, "%d", &distance) == 1) {
            if (distance >= -10 && distance <= 10) {
                use_block(player_index, distance);
            } else {
                printf("距离必须在 -10 到 10 之间\n");
            }
        } else {
            printf("用法: block n (n为距离)\n");
        }
        return 0; // 回合继续
    }
    else if (strcmp(command, "robot") == 0) {
        use_doll(player_index, 0); // 使用机器娃娃，距离参数为0
        return 0; // 回合继续
    }
    else if (strncmp(command, "step", 4) == 0) {
        // 测试指令，不显示在帮助中
        int steps;
        if (sscanf(command + 4, "%d", &steps) == 1) {
            printf("测试指令: 前进 %d 步\n", steps);
            move_player(player_index, steps);
            handle_position(player_index);
        } 
        else {
            printf("用法: step n (n为步数)\n");
        }
        return 1; // 回合结束
    }
    else if (strncmp(command, "sell", 4) == 0) {
        int position;
        if (sscanf(command + 5, "%d", &position) == 1) {
            if (position >= 0 && position < TOTAL_CELLS) {
                sell_property(player_index, position);
            } else {
                printf("地产位置必须在 0 到 %d 之间\n", TOTAL_CELLS - 1);
            }
        } else {
            printf("用法: sell n (n为地产的一维坐标)\n");
        }
        return 0; // 回合继续
    }
    else if (strcmp(command, "quit") == 0) {
        printf("游戏提前结束，开始结算...\n");
        game_state.ended = true;
        return 2;
    }
    else if (strcmp(command, "dump") == 0){
        dump_json(player_count);
        return 2; 
    }
    else {
        printf("未知命令。输入 'help' 查看可用命令。\n");
        return 0; // 回合继续
    }
}

// 主游戏循环
void game_loop() {
    srand(time(NULL)); // 初始化随机数种子
    
    while (!game_state.ended) {
        display_map();
        display_player_status(game_state.now_player);
        int row, col;
        position_to_coord(players[game_state.now_player].location, &row, &col);
        int pos = coord_to_position(row, col);


        if (!players[game_state.now_player].alive) {
            printf("%s 已出局，跳过本回合\n", players[game_state.now_player].name);
        } else if (players[game_state.now_player].buff->hospitail > 0) {
            players[game_state.now_player].buff->hospitail--;
            printf("%s 住院中，跳过本回合\n", players[game_state.now_player].name);
        } else if (players[game_state.now_player].buff->prison > 0) {
            players[game_state.now_player].buff->prison--;
            printf("%s 监禁中，跳过本回合\n", players[game_state.now_player].name);
        } else if (pos != -1 && placed_props.barrier[pos] > 0) {
            printf("%s 被路障阻拦，停止一回合\n", players[game_state.now_player].name);
            placed_props.barrier[pos] = 0;
        } else {
            bool turn_end = false;
            char command[50];
            
            printf("\n%s 的回合，请输入命令 (输入 'help' 查看帮助):\n", players[game_state.now_player].name);
            
            while (!turn_end) {
                printf(">");
                if (fgets(command, sizeof(command), stdin) == NULL) {
                    // 处理错误或EOF
                    continue;
                }

                // 去除换行符
                command[strcspn(command, "\n")] = 0;
                    
                int command_result = process_command(game_state.now_player, command);
                    
                if (command_result == 2) {// 立即终止程序
                    return;
                } else if (command_result == 1) {// 回合结束
                    turn_end = true;
                }
                // 移除了额外的提示，因为每次循环都会显示 ">"
            }
        }
        
        // 更新财神附身状态
        if (players[game_state.now_player].buff->god > 0) {
            players[game_state.now_player].buff->god--;
        }
        
        // 检查游戏是否结束（简化版：当只剩一个玩家时结束）
        int alive_count = 0;
        int last_alive = -1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].alive) {
                alive_count++;
                last_alive = i;
            }
        }
        
        if (alive_count <= 1) {
            game_state.ended = true;
            if (alive_count == 1) {
                game_state.winner = last_alive;
                printf("\n游戏结束！%s 获胜！\n", players[last_alive].name);
            } else {
                printf("\n游戏结束！所有玩家都出局了！\n");
            }
        }
        
        // 切换到下一个玩家
        game_state.now_player = (game_state.now_player + 1) % MAX_PLAYERS;
        while (!players[game_state.now_player].alive && !game_state.ended) {
            game_state.now_player = (game_state.now_player + 1) % MAX_PLAYERS;
        }
    }
}
// 主函数
int main(int argc, char *argv[]) {
    char preset_path[100] = {0};

    if (argc > 1) {
        // 构建 preset.json 和 dump.json 的路径
        snprintf(preset_path, sizeof(preset_path), "%s/preset.json", argv[1]);
        snprintf(save_path, sizeof(save_path), "%s/dump.json", argv[1]);

        // 创建目录
        make_dirs(argv[1]);
    } else {
        // 如果没有命令行参数，使用当前目录
        strcpy(save_path, "dump.json");
        strcpy(preset_path, "preset.json");
    }

    // 检查 preset.json 是否存在
    FILE *preset_file = fopen(preset_path, "r");
    if (preset_file != NULL) {
        fclose(preset_file);
        if (import_json(preset_path)) {
            printf("导入存档成功，直接开始游戏。\n");
            game_loop();
            return 0;
        } else {
            printf("导入存档失败，开始新游戏。\n");
        }
    }

    // 正常游戏初始化
    printf("欢迎来到大富翁游戏！\n");
    init_map();

    int initial_fund = 10000;
    char fund_input[20];
    while(1){
        printf("请设置初始资金 (1000-50000，默认10000): ");
        if (fgets(fund_input, sizeof(fund_input), stdin) != NULL) {
            if (fund_input[0] != '\n') { // 用户输入了内容
                int input_fund = atoi(fund_input);
                if (input_fund >= 1000 && input_fund <= 50000) {
                    initial_fund = input_fund;
                    break;
                } else {
                    printf("请输入范围内的数额 (1000-50000)\n");
                }
            }
        }
    }
    char player_chars[5];
    int valid_input = 0;
    
    printf("请选择玩家 (1-钱夫人 2-阿土伯 3-孙小美 4-金贝贝，如输入12表示选择钱夫人和阿土伯): ");
    while (!valid_input) {
        if (fgets(player_chars, sizeof(player_chars), stdin) == NULL) {
            continue;
        }
        
        // 去除换行符
        player_chars[strcspn(player_chars, "\n")] = 0;
        
        int len = strlen(player_chars);
        if (len < 2 || len > 4) {
            printf("非法输入: 请选择2-4个玩家\n");
            printf("请重新选择玩家: ");
            continue;
        }
        
        valid_input = 1;
        int used[5] = {0}; // 检查重复数字
        
        for (int i = 0; i < len; i++) {
            char c = player_chars[i];
            if (c < '1' || c > '4') {
                printf("非法输入: 只能输入数字1-4\n");
                valid_input = 0;
                break;
            }
            
            int num = c - '0';
            if (used[num]) {
                printf("非法输入: 不能选择重复角色\n");
                valid_input = 0;
                break;
            }
            used[num] = 1;
        }
        
        if (!valid_input) {
            printf("请重新选择玩家: ");
        }
    }


    init_players(player_chars, 10000);

    show_help();

    game_loop();

    if (game_state.quit_early) {
        // 结算代码
        printf("\n=== 游戏提前结束 ===\n");
        printf("开始结算玩家资产...\n\n");

        int max_fund = -1;
        int winner_index = -1;

        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].alive) continue;

            int total_assets = players[i].fund;
            for (int j = 0; j < TOTAL_CELLS; j++) {
                int row, col;
                position_to_coord(j, &row, &col);
                if (map[row][col].houses != NULL && 
                    map[row][col].houses->owner == players[i].symbol){
                    total_assets += map[row][col].price * map[row][col].houses->level;
                }
            }
            total_assets += players[i].items->barrier * 50;
            total_assets += players[i].items->bomb * 50;
            total_assets += players[i].items->robot * 30;

            printf("%s 的总资产: %d元\n", players[i].name, total_assets);

            if (total_assets > max_fund) {
                max_fund = total_assets;
                winner_index = i;
            }
        }

        if (winner_index != -1) {
            printf("\n胜者是: %s！总资产: %d元\n", players[winner_index].name, max_fund);
        }
    }

    printf("\n游戏结束，按任意键退出...");
    getchar();
    getchar();

    return 0;
}