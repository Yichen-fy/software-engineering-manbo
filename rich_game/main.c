#include "ustil.h"
// 主游戏循环
void game_loop() {
    srand(time(NULL));
    
    printf("欢迎来到大富翁简化版游戏!\n");
    
    // 设置玩家数量和初始资金
    int initial_money = 10000;
    printf("请输入玩家数量 (2-4): ");
    scanf("%d", &player_count);
    
    if (player_count < 2 || player_count > 4) {
        printf("玩家数量必须在2-4之间，已设置为2\n");
        player_count = 2;
    }
    
    printf("请输入初始资金 (默认10000): ");
    scanf("%d", &initial_money);
    
    if (initial_money < 1000 || initial_money > 50000) {
        printf("初始资金必须在1000-50000之间，已设置为10000\n");
        initial_money = 10000;
    }
    
    // 初始化游戏
    init_map();
    init_players(player_count, initial_money);
    
    printf("游戏开始! 初始资金: %d元\n", initial_money);
    
    // 游戏主循环
    while (!game_over) {
        Player *current = &players[current_player];
        
        // 跳过住院或监禁的玩家
        if (current->hospitalized > 0) {
            printf("\n%s 正在住院，跳过本回合 (%d回合后出院)\n", 
                   current->name, current->hospitalized);
            current->hospitalized--;
            current_player = (current_player + 1) % player_count;
            continue;
        }
        
        if (current->imprisoned > 0) {
            printf("\n%s 正在监禁中，跳过本回合 (%d回合后释放)\n", 
                   current->name, current->imprisoned);
            current->imprisoned--;
            current_player = (current_player + 1) % player_count;
            continue;
        }
        
        // 财神模式递减
        if (current->god_mode > 0) {
            current->god_mode--;
        }
        
        printf("\n轮到 %s 的回合\n", current->name);
        display_player_status(current_player);
        
        char command[20];
        int steps;
        
        while (1) {
            display_map();
            printf("\n请输入命令 (输入help查看帮助): ");
            scanf("%s", command);
            
            if (strcasecmp(command, "step") == 0){
                scanf("%d", &steps);
                printf("移动 %d 步", steps);
                move_player(current_player, steps);
                handle_position(current_player);
                break;
            }
            else if (strcasecmp(command, "roll") == 0) {
                steps = roll_dice();
                printf("掷出了 %d 点\n", steps);
                move_player(current_player, steps);
                handle_position(current_player);
                display_map();
                break;
            } else if (strcasecmp(command, "block") == 0) {
                int distance;
                scanf("%d", &distance);
                if (distance >= -10 && distance <= 10 && distance != 0) {
                    use_block(current_player, distance);
                    display_map();
                } else {
                    printf("距离必须在-10到10之间且不能为0\n");
                }
            } else if (strcasecmp(command, "bomb") == 0) {
                int distance;
                scanf("%d", &distance);
                if (distance >= -10 && distance <= 10 && distance != 0) {
                    use_bomb(current_player, distance);
                    display_map();
                } else {
                    printf("距离必须在-10到10之间且不能为0\n");
                }
            } else if (strcasecmp(command, "robot") == 0) {
                use_robot(current_player);
                display_map();
            } else if (strcasecmp(command, "query") == 0) {
                display_player_status(current_player);
            } else if (strcasecmp(command, "help") == 0) {
                show_help();
            } else if (strcasecmp(command, "quit") == 0) {
                game_over = 1;
                break;
            } else {
                printf("未知命令，请输入help查看帮助\n");
            }
            display_map();
        }
        
        // 检查游戏是否结束
        if (current->money < 0) {
            printf("%s 破产了！游戏结束\n", current->name);
            game_over = 1;
        }
        
        // 切换到下一个玩家
        current_player = (current_player + 1) % player_count;
    }
    
    printf("游戏结束!\n");
}

int main() {
    game_loop();
    return 0;
}