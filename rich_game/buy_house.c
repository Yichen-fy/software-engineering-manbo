#include "ustil.h"

// 购买地产
void buy_property(int player_index) {
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    
    if (map[row][col].type != 'O') {
        printf("此处不能购买地产\n");
        return;
    }
    
    if (map[row][col].owner != -1) {
        printf("此地已有主人\n");
        return;
    }
    
    if (players[player_index].money >= map[row][col].price) {
        players[player_index].money -= map[row][col].price;
        map[row][col].owner = player_index;
        players[player_index].properties[players[player_index].property_count++] = players[player_index].position;
        printf("%s 购买了位置 (%d, %d) 的地产，花费 %d元\n", 
               players[player_index].name, row, col, map[row][col].price);
    } else {
        printf("资金不足，无法购买此地产\n");
    }
}

// 升级地产
void upgrade_property(int player_index) {
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    
    if (map[row][col].owner != player_index) {
        printf("这不是你的地产\n");
        return;
    }
    
    if (map[row][col].level < 3 && players[player_index].money >= map[row][col].price) {
        players[player_index].money -= map[row][col].price;
        map[row][col].level++;
        printf("%s 升级了位置 (%d, %d) 的地产，现在是 %d 级，花费 %d元\n", 
               players[player_index].name, row, col, map[row][col].level, map[row][col].price);
    } else {
        printf("无法升级此地产\n");
    }
}

// 支付过路费
void pay_toll(int player_index) {
    int row, col;
    position_to_coord(players[player_index].position, &row, &col);
    
    int owner = map[row][col].owner;
    int toll = map[row][col].toll * (map[row][col].level + 1);
    
    if (players[player_index].god_mode > 0) {
        printf("财神附身，免付过路费\n");
        return;
    }
    
    if (players[owner].hospitalized > 0 || players[owner].imprisoned > 0) {
        printf("地主正在医院或监狱中，免付过路费\n");
        return;
    }
    
    if (players[player_index].money >= toll) {
        players[player_index].money -= toll;
        players[owner].money += toll;
        printf("%s 向 %s 支付了过路费 %d元\n", 
               players[player_index].name, players[owner].name, toll);
    } else {
        printf("%s 资金不足，无法支付过路费，破产了！\n", players[player_index].name);
        game_over = 1;
    }
}
