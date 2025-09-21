#include "ustil.h"

// 显示玩家状态
void display_player_status(int player_index) {
    printf("\n%s 的状态:\n", players[player_index].name);
    printf("资金: %d元\n", players[player_index].money);
    printf("点数: %d点\n", players[player_index].points);
    
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    printf("位置: (%d, %d)\n", row, col);
    
    printf("地产: %d处\n", players[player_index].property_count);
    printf("道具: %d个\n", players[player_index].item_count);
    
    if (players[player_index].hospitalized > 0) {
        printf("状态: 住院中 (%d回合后出院)\n", players[player_index].hospitalized);
    } else if (players[player_index].imprisoned > 0) {
        printf("状态: 监禁中 (%d回合后释放)\n", players[player_index].imprisoned);
    } else if (players[player_index].god_mode > 0) {
        printf("状态: 财神附身 (%d回合有效)\n", players[player_index].god_mode);
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
    players[player_index].position = (players[player_index].position + steps) % (2 * (MAP_ROWS + MAP_COLS - 2));
    
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    printf("%s 移动了 %d 步，到达位置 (%d, %d)\n", 
           players[player_index].name, steps, row, col);
}

// 处理玩家到达的位置
void handle_position(int player_index) {
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    Cell *cell = &map[row][col];
    
    printf("%s 到达了位置 (%d, %d): ", players[player_index].name, row, col);
    
    switch (cell->type) {
        case 'S':
            printf("起点\n");
            break;
            
        case 'O':
            if (cell->owner == -1) {
                printf("空地，可以购买\n");
                printf("购买价格: %d元\n", cell->price);
                printf("是否购买? (y/n): ");
                char choice;
                scanf(" %c", &choice);
                if (tolower(choice) == 'y') {
                    buy_property(player_index);
                }
            } else if (cell->owner == player_index) {
                printf("自己的地产，可以升级\n");
                printf("升级价格: %d元\n", cell->price);
                printf("是否升级? (y/n): ");
                char choice;
                scanf(" %c", &choice);
                if (tolower(choice) == 'y') {
                    upgrade_property(player_index);
                }
            } else {
                printf("%s 的地产，需要支付过路费\n", players[cell->owner].name);
                printf("过路费: %d元\n", cell->toll * (cell->level + 1));
                pay_toll(player_index);
            }
            break;

        case 'T':
            printf("道具屋\n价格如下所示：\n");
            printf("1. 路障 50点数\n");
            printf("2. 机器娃娃 30点数\n");
            printf("3. 炸弹 50点数\n");
            // 根据输入的序号获得对应的道具并扣除相应的点数
            int item = 0;
            int cost = 0;
            printf("请输入道具编号: ");
            scanf("%d", &item);
            switch (item) {
                case 1: cost = 50; break;
                case 2: cost = 30; break;
                case 3: cost = 50; break;
                default: printf("无效的道具编号\n"); return;
            }
            if (players[player_index].points >= cost) {
                players[player_index].points -= cost;
                players[player_index].items[players[player_index].item_count++] = item;
                const char *item_name = "";
                switch (item) {
                    case 1: item_name = "路障"; break;
                    case 2: item_name = "机器娃娃"; break;
                    case 3: item_name = "炸弹"; break;
                }
                printf("获得了 %s\n", item_name);
            } else if (players[player_index].points < cost) {
                printf("点数不足，无法购买道具\n");
            } else if (players[player_index].item_count >= MAX_ITEMS) {
                printf("道具栏已满，无法获得新道具\n");
            }
            break;
            
        case 'G':
            printf("礼品屋\n");
            // 简化版：随机获得一个礼品
            int gift = rand() % 3 + 1;
            switch (gift) {
                case 1:
                    players[player_index].money += 2000;
                    printf("获得了 2000元奖金\n");
                    break;
                case 2:
                    players[player_index].points += 200;
                    printf("获得了 200点\n");
                    break;
                case 3:
                    players[player_index].god_mode = 5;
                    printf("获得了财神附身，5回合有效\n");
                    break;
            }
            break;
            
        case 'M':
            printf("魔法屋\n");
            // 简化版：随机获得或失去一些资源
            int effect = rand() % 3;
            switch (effect) {
                case 0:
                    players[player_index].money += 1000;
                     printf("获得了 1000元\n");
                    break;
                case 1:
                    players[player_index].points += 100;
                    printf("获得了 100点\n");
                    break;
                case 2:
                    players[player_index].money -= 500;
                    printf("失去了 500元\n");
                    break;
                }
                break;

        case '$':
            printf("矿地\n");
            // 获得点数
            int points = 20 + rand() % 80;
            players[player_index].points += points;
            printf("获得了 %d 点\n", points);
            break;

            
        case 'H':
            printf("医院\n");
            if (players[player_index].hospitalized == 0) {
                printf("只是路过医院\n");
            } else {
                printf("正在医院接受治疗\n");
            }
            break;
            
        case 'P':
            printf("监狱\n");
            if (players[player_index].imprisoned == 0) {
                printf("只是路过监狱\n");
            } else {
                printf("正在监狱服刑\n");
            }
            break;
            
        default:
            printf("未知地点\n");
            break;
    }
    
    // 检查是否有道具效果
    if (cell->has_item) {
        printf("触发了道具效果: ");
        switch (cell->item_type) {
            case 1: // 路障
                printf("被路障拦截，停止一回合\n");
                // 简化版：跳过下一回合
                break;
            case 3: // 炸弹
                printf("被炸弹炸伤，住院3天\n");
                players[player_index].hospitalized = 3;
                break;
        }
        cell->has_item = 0; // 移除道具
    }
}

// 使用路障
void use_block(int player_index, int distance) {
    if (players[player_index].item_count > 0) {
        int has_block = 0;
        for (int i = 0; i < players[player_index].item_count; i++) {
            if (players[player_index].items[i] == 1) {
                has_block = 1;
                // 移除道具
                for (int j = i; j < players[player_index].item_count - 1; j++) {
                    players[player_index].items[j] = players[player_index].items[j+1];
                }
                players[player_index].item_count--;
                break;
            }
        }
        
        if (has_block) {
            int target_pos = (players[player_index].position + distance + (2 * (MAP_ROWS + MAP_COLS - 2))) % (2 * (MAP_ROWS + MAP_COLS - 2));
            int row, col;
            position_to_coord(target_pos, &row, &col);
            map[row][col].has_item = 1;
            map[row][col].item_type = 1;
            printf("在位置 (%d, %d) 放置了路障\n", row, col);
        } else {
            printf("没有路障道具\n");
        }
    } else {
        printf("没有可用道具\n");
    }
}

// 使用炸弹
void use_bomb(int player_index, int distance) {
    if (players[player_index].item_count > 0) {
        int has_bomb = 0;
        for (int i = 0; i < players[player_index].item_count; i++) {
            if (players[player_index].items[i] == 3) {
                has_bomb = 1;
                // 移除道具
                for (int j = i; j < players[player_index].item_count - 1; j++) {
                    players[player_index].items[j] = players[player_index].items[j+1];
                }
                players[player_index].item_count--;
                break;
            }
        }
        
        if (has_bomb) {
            int target_pos = (players[player_index].position + distance + (2 * (MAP_ROWS + MAP_COLS - 2))) % (2 * (MAP_ROWS + MAP_COLS - 2));
            int row, col;
            position_to_coord(target_pos, &row, &col);
            map[row][col].has_item = 1;
            map[row][col].item_type = 3;
            printf("在位置 (%d, %d) 放置了炸弹\n", row, col);
        } else {
            printf("没有炸弹道具\n");
        }
    } else {
        printf("没有可用道具\n");
    }
}

// 使用机器娃娃
void use_robot(int player_index) {
    if (players[player_index].item_count > 0) {
        int has_robot = 0;
        for (int i = 0; i < players[player_index].item_count; i++) {
            if (players[player_index].items[i] == 2) {
                has_robot = 1;
                // 移除道具
                for (int j = i; j < players[player_index].item_count - 1; j++) {
                    players[player_index].items[j] = players[player_index].items[j+1];
                }
                players[player_index].item_count--;
                break;
            }
        }
        
        if (has_robot) {
            printf("清除了前方10格内的道具\n");
            for (int i = 1; i <= 10; i++) {
                int target_pos = (players[player_index].position + i) % (2 * (MAP_ROWS + MAP_COLS - 2));
                int row, col;
                position_to_coord(target_pos, &row, &col);
                map[row][col].has_item = 0;
            }
        } else {
            printf("没有机器娃娃道具\n");
        }
    } else {
        printf("没有可用道具\n");
    }
}

// 显示帮助信息
void show_help() {
    printf("\n可用命令:\n");
    printf("roll        - 掷骰子移动\n");
    printf("block n     - 在前后n格放置路障\n");
    printf("bomb n      - 在前后n格放置炸弹\n");
    printf("robot       - 使用机器娃娃清除前方道路\n");
    printf("query       - 查看自己的资产\n");
    printf("map         - 显示地图\n");
    printf("help        - 显示帮助信息\n");
    printf("quit        - 退出游戏\n");
    printf("step n        - 向前移动n步\n");
}
