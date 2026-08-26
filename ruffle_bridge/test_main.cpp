#include "ruffle_bridge.h"
#include <cstdio>
#include <vector>
#include <fstream>
#include <cassert>

int main() {
    std::ifstream file("Minecraft.Client/Common/Media/PauseMenu720.swf", std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open PauseMenu720.swf\n");
        return 1;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        fprintf(stderr, "Failed to read PauseMenu720.swf\n");
        return 1;
    }

    const char *searchDirs[] = {
        "Minecraft.Client/Common/Media",
        "Minecraft.Client/Windows64Media/Media",
        "Common/Media",
        "Windows64Media/Media",
    };

    RuffleBridgePlayer *player = nullptr;
    RuffleBridgeStatus status = ruffle_bridge_player_create(
        1280, 720,
        buffer.data(), buffer.size(),
        "file:///Minecraft.Client/Common/Media/PauseMenu720.swf",
        searchDirs, 4,
        &player
    );

    if (status != RUFFLE_BRIDGE_OK || !player) {
        fprintf(stderr, "ruffle_bridge_player_create failed (%d)\n", (int)status);
        return 1;
    }
    printf("Successfully created Ruffle player for PauseMenu720.swf!\n");

    // Test resolving Button1
    RuffleBridgeButton *btn1 = nullptr;
    status = ruffle_bridge_button_create("Button1", &btn1);
    printf("ruffle_bridge_button_create Button1 status: %d\n", (int)status);

    if (btn1) {
        status = ruffle_bridge_button_set_label(player, btn1, "Resume Game");
        printf("ruffle_bridge_button_set_label status: %d\n", (int)status);

        status = ruffle_bridge_button_set_enabled(player, btn1, true);
        printf("ruffle_bridge_button_set_enabled status: %d\n", (int)status);
    }

    ruffle_bridge_player_destroy(player);
    return 0;
}
