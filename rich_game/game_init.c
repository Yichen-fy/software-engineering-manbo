#include "ustil.h"
// 初始化玩家
void init_players(int count, int initial_money) {
    player_count = count;
    
    // 初始化玩家属性
    for (int i = 0; i < count; i++) {
        players[i].money = initial_money;
        players[i].points = 0;
        players[i].position = 0;
        players[i].property_count = 0;
        players[i].item_count = 0;
        players[i].hospitalized = 0;
        players[i].imprisoned = 0;
        players[i].god_mode = 0;
        
        for (int j = 0; j < MAX_PROPERTIES; j++) {
            players[i].properties[j] = -1;
        }
        
        for (int j = 0; j < MAX_ITEMS; j++) {
            players[i].items[j] = 0;
        }
    }
    
    // 设置玩家名称和符号
    strcpy(players[0].name, "钱夫人");
    players[0].symbol = 'Q';
    
    strcpy(players[1].name, "阿土伯");
    players[1].symbol = 'A';
    
    strcpy(players[2].name, "孙小美");
    players[2].symbol = 'X';
    
    strcpy(players[3].name, "金贝贝");
    players[3].symbol = 'J';
    
    current_player = 0;
    game_over = 0;
}
