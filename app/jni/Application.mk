ANDROID_JPEG_NO_ASSEMBLER := true
APP_PLATFORM=android-10
# ARMv7 is significanly faster due to the use of the hardware FPU
APP_STL := gnustl_shared
APP_ABI := armeabi-v7a
#APP_OPTIM := debug
APP_CPPFLAGS += -fexceptions -frtti
