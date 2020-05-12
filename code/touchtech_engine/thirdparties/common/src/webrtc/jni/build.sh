/Users/brucewang/Documents/adt-bundle-mac/android-ndk-r10c/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc -I-isystem -I/Users/brucewang/Documents/adt-bundle-mac/android-ndk-r10c/platforms/android-3/arch-arm/usr/include -I../.. -I../../../../.. -S -o nsx_core_neon_offsets.S ../modules/audio_processing/ns/nsx_core_neon_offsets.c

python ./generate_asm_header.py nsx_core_neon_offsets.S nsx_core_neon_offsets.h offset_nsx_

/Users/brucewang/Documents/adt-bundle-mac/android-ndk-r10c/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc -I-isystem -I/Users/brucewang/Documents/adt-bundle-mac/android-ndk-r10c/platforms/android-3/arch-arm/usr/include -I../.. -I../../../../.. -S -o aecm_core_neon_offsets.S ../modules/audio_processing/aecm/aecm_core_neon_offsets.c

python ./generate_asm_header.py aecm_core_neon_offsets.S aecm_core_neon_offsets.h offset_aecm_