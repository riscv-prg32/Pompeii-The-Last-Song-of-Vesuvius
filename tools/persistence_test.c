#include "../src/game.h"
#include <assert.h>
#include <stdio.h>

static AreliaGame game;
int main(void) {
    game_init(&game);
    /* A key collected in the Forum must not respawn on room reload. */
    game.mode = 1;
    game.x = 16;
    game.y = 5;
    game_step(&game, 0);
    assert(game.keys == 1);
    assert(game.collected_keys[0] == 1);
    /* Travel through the north exit and back. */
    game.x = 9; game.y = 1; game.move_cooldown = 0;
    game_step(&game, ARELIA_INPUT_UP);
    assert(game.room == 1);
    game.x = 9; game.y = 10; game.move_cooldown = 0;
    game_step(&game, ARELIA_INPUT_DOWN);
    assert(game.room == 0);
    assert(game.tiles[5][16] == 0);
    assert(game.keys == 1);
    /* A defeated enemy stays defeated when the player revisits a room. */
    game.defeated_enemies[0] |= 1u;
    game.x = 9; game.y = 1; game.move_cooldown = 0;
    game_step(&game, ARELIA_INPUT_UP);
    game.x = 9; game.y = 10; game.move_cooldown = 0;
    game_step(&game, ARELIA_INPUT_DOWN);
    assert(game.room == 0);
    assert(game.enemies[0].active == 0);
    /* A new game resets all persistence state. */
    game_init(&game);
    assert(game.collected_keys[0] == 0);
    assert(game.defeated_enemies[0] == 0);
    assert(game.tiles[5][16] == 3);
    assert(game.enemies[0].active == 1);
    puts("Persistence regression test passed");
    return 0;
}
