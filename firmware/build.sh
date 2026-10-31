#!./bin/zsh
west build -b openboardlink/stm32f103xb "/Users/zadock/Git-Repos/USB2SWD/firmware" -p \
        --build-dir "/Users/zadock/Git-Repos/USB2SWD/firmware/build/openboardlink/stm32f103xb" \
        -- -DBOARD_ROOT='/Users/zadock/Git-Repos/USB2SWD/firmware' \
        -DCONFIG_DEBUG_OPTIMIZATIONS=y -DCONFIG_DEBUG_THREAD_INFO=y -DCMAKE_EXPORT_COMPILE_COMMANDS=ON