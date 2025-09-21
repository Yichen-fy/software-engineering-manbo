#include "ustil.h"
// 初始化地图为方形边界
void init_map() {
    // 初始化所有格子为'-'
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            map[i][j].type = ' ';
            map[i][j].owner = -1;
            map[i][j].level = '0';
            map[i][j].has_item = 0;
            map[i][j].price = 0;
            map[i][j].toll = 0;
        }
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
    
    // 设置特殊地点
    map[0][0].type = 'S'; // 起点
    map[0][15].type = 'H'; // 医院
    map[0][MAP_COLS-1].type = 'T'; // 道具屋
    
    map[MAP_ROWS-1][0].type = 'M'; // 魔法屋
    map[MAP_ROWS-1][15].type = 'P'; // 监狱
    map[MAP_ROWS-1][MAP_COLS-1].type = 'G'; // 礼品屋
    
    // 设置矿地
    map[1][0].type = '$';
    map[2][0].type = '$';
    map[3][0].type = '$';
    map[4][0].type = '$';
    map[5][0].type = '$';
    map[6][0].type = '$';
}



// 将一维位置转换为二维坐标,确保了玩家沿着矩形边界顺时针移动，符合大富翁游戏的传统玩法
void position_to_coord(int position, int *row, int *col) {
    int total_cells = 2 * (MAP_ROWS + MAP_COLS - 2); // 边界上的总格子数
    
    position = position % total_cells;
    
    if (position < MAP_COLS) {
        // 顶部行
        *row = 0;
        *col = position;
    } else if (position < MAP_COLS + MAP_ROWS - 2) {
        // 右侧列
        *row = position - MAP_COLS + 1;
        *col = MAP_COLS - 1;
    } else if (position < 2 * MAP_COLS + MAP_ROWS - 2) {
        // 底部行（从右到左）
        *row = MAP_ROWS - 1;
        *col = MAP_COLS - 1 - (position - (MAP_COLS + MAP_ROWS - 2));
    } else {
        // 左侧列（从下到上）
        *row = MAP_ROWS - 1 - (position - (2 * MAP_COLS + MAP_ROWS - 2)) - 1;
        *col = 0;
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
            for (int k = 0; k < player_count; k++) {
                int row, col;
                position_to_coord(players[k].position, &row, &col);
                if (row == i && col == j) {
                    player_here = k;
                    break;
                }
            }
            
            if (player_here != -1) {
                printf("%c", players[player_here].symbol);
            } else if (map[i][j].has_item) {
                switch (map[i][j].item_type) {
                    case 1: printf("#"); break; // 路障
                    case 2: printf("@"); break; // 机器娃娃
                    case 3: printf("@"); break; // 炸弹
                    default: printf("?");
                }
            } else {
                switch (map[i][j].type) {
                    case 'S': printf("S"); break;
                    case 'O': 
                       if (map[i][j].owner == -1) {
                            printf("O");
                        } else {
                            switch (map[i][j].owner)
                            {
                            case 0:
                                printf("\033[1;31m%c\033[0m", map[i][j].level);
                                break;
                            case 1:
                                printf("\033[1;32m%c\033[0m", map[i][j].level);
                                break;
                            case 2:
                                printf("\033[1;34m%c\033[0m", map[i][j].level); 
                                break;
                            case 3:
                                printf("\033[1;33m%c\033[0m", map[i][j].level); 
                                break;
                            default:
                                break;
                            }
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
        }
        printf("\n");
    }
    
    printf("------------------------------------------------------------\n");
    printf("图例: S-起点 O-空地 T-道具屋 G-礼品屋 M-魔法屋/矿地 H-医院 P-监狱\n");
    printf("      数字-地产等级(0-3) Q-钱夫人 A-阿土伯 S-孙小美 J-金贝贝\n");
    printf("      #-路障 @-炸弹/机器娃娃\n");
}
