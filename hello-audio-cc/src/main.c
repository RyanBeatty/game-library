#ifndef UNITY_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

#endif

int main(void) {
    InitAudio();

    // char cwd[PATH_MAX];
    // getcwd(cwd, sizeof(cwd));
    // printf("Current working directory: %s\n", cwd);

    SoundId s1 = LoadLSoundTest("foo");
    SoundId s2 = LoadLSound("src/beep.wav");

    {
        while (true) {
            PlayLSound(s1, 0);
            PlayLSound(s2, 0);
            sleep(3);
        }
    }
    return 0;
}
