#ifndef LIBTERMUX_CORE__NOS__C__UNIX_SELINUX_UTILS___H
#define LIBTERMUX_CORE__NOS__C__UNIX_SELINUX_UTILS___H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 * Regex to match SELinux context of a process on Android.
 *
 * The security context label is in the format `user:role:type:sensitivity[:categories]`.
 * For non system apps, it will be something like `u:r:untrusted_app:s0:c159,c256,c512,c768`.
 *
 * The `user` is always `u`.
 *
 * The role is always `r` for subjects (example: processes) and `object_r`
 * for objects (example: files).
 *
 * The `type` depends on the app type. The common ones are the following.
 * - `untrusted_app`: untrusted app processes running with `targetSdkVersion`
 *   equal to android sdk version or with `targetSdkVersion` `>=` than
 *   the max `targetSdkVersion` of the highest `untrusted_app_*`
 *   backward compatibility domains.
 * - `untrusted_app_25`: untrusted app processes running with `targetSdkVersion` `<= 25`.
 * - `untrusted_app_27`: untrusted app processes running with `targetSdkVersion` `26-28`.
 * - `untrusted_app_27`: untrusted app processes running with `26 <= targetSdkVersion` `<= 28`.
 * - `untrusted_app_29`: untrusted app processes running with `targetSdkVersion` `= 29`.
 * - `untrusted_app_30`: untrusted app processes running with `targetSdkVersion` `30-31`.
 * - `untrusted_app_32`: untrusted app processes running with `targetSdkVersion` `32-33`.
 * - `system_app`: system app processes that are signed with the platform
 *   key and use `sharedUserId=com.android.system`.
 * - `platform_app`: platform app processes that are signed with the platform
 *   key and do not use `sharedUserId=com.android.system`.
 * - `priv_app`: privileged app processes that are not signed with the
 *   platform key.
 *
 * The `sensitivity` is always `s0`.
 *
 * The Multi-Category Security (MCS) `categories` are optional, but if set,
 * then either `2` or `4` categories are set where each category starts with
 * `c` followed by a number. The `untrusted_app_25` should have 2 categories,
 * while `>=` `untrusted_app_27` should have 4 categories if app has
 * `targetSdkVersion` `>= 28`. For example for uid `10159` and user `0`
 * security context with be set to `u:r:untrusted_app_27:s0:c159,c256,c512,c768`
 * and for uid `1010159` and user `10` it will be set to
 * `u:r:untrusted_app_27:s0:c159,c256,c522,c768`, note `c512` vs `c522`.
 * This is because `untrusted_app_25` selinux domain uses `levelFrom=user`
 * selector in SELinux `seapp_contexts`, which adds two category types.
 *
 * - https://source.android.com/docs/security/features/selinux/concepts#security_contexts
 * - https://cs.android.com/android/platform/superproject/+/bea25463132c3f4bb35816d175bbe8551f11fc9d:system/sepolicy/README.apps.md
 * - https://github.com/termux/termux-app/issues/3167#issuecomment-1369708339
 * - https://github.com/SELinuxProject/selinux-notebook/blob/main/src/mls_mcs.md#multi-level-and-multi-category-security
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/seapp_contexts
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/untrusted_app.te;l=13-14
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r9:external/selinux/libselinux/src/context.c;l=17
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r9:external/selinux/libselinux/src/android/android_seapp.c;l=686
 */
#define REGEX__PROCESS_CONTEXT "^u:r:[^\n\t\r :]+:s0(:c[0-9]+,c[0-9]+(,c[0-9]+,c[0-9]+)?)?$"

/**
 * The process context prefix assigned to untrusted app processes
 * running with `targetSdkVersion <= 25`.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/private/seapp_contexts;l=176
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/public/untrusted_app.te;l=31
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:/system/sepolicy/private/untrusted_app_25.te
 */
#define PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25 "u:r:untrusted_app_25:"

/**
 * The process context prefix assigned to untrusted app processes
 * running with `26 <= targetSdkVersion <= 28`.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/private/seapp_contexts;l=174
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/public/untrusted_app.te;l=28
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/private/untrusted_app_27.te
 */
#define PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27 "u:r:untrusted_app_27:"

/**
 * The process context prefix assigned to app processes
 * running inside the `run-as` command from ADB or root shell.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/private/seapp_contexts;l=177
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/public/runas_app.te;l=1
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r18:system/sepolicy/private/runas_app.te
 */
#define PROCESS_CONTEXT_PREFIX__RUNAS_APP "u:r:runas_app:"

/**
 * The process context assigned to `root` (`0`) user processes by
 * AOSP, like for `adb root`. The process context always equals
 * `u:r:su:s0` without any categories.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/su.te
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/core/rootdir/init.usb.rc;l=15
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/init.te;l=32
 */
#define PROCESS_CONTEXT__AOSP_SU "u:r:su:s0"

/**
 * The process context assigned to `root` (`0`) user processes by
 * KernelSU, like for `su` commands. The process context always equals
 * `u:r:ksu:s0` without any categories.
 *
 * - https://github.com/tiann/KernelSU
 * - https://github.com/tiann/KernelSU/blob/v3.2.5/manager/app/src/main/java/me/weishu/kernelsu/Natives.kt#L29
 * - https://github.com/tiann/KernelSU/blob/v3.2.5/website/docs/guide/app-profile.md#selinux
 */
#define PROCESS_CONTEXT__KERNEL_SU "u:r:ksu:s0"

/**
 * The process context type assigned to `root` (`0`) user processes by
 * Magisk, like for `su` commands. The process context always equals
 * `u:r:magisk:s0` without any categories.
 *
 * - https://github.com/topjohnwu/Magisk
 * - https://github.com/topjohnwu/Magisk/blob/master/native/src/include/consts.hpp#L36
 * - https://github.com/topjohnwu/Magisk/blob/master/docs/tools.md#magiskpolicy
 */
#define PROCESS_CONTEXT__MAGISK_SU "u:r:magisk:s0"

/**
 * The process context assigned to `shell` (`2000`) user, like for
 * `adb shell`. The process context always equals `u:r:shell:s0`
 * without any categories.
 *
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/seapp_contexts;l=168
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/sepolicy/private/shell.te
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:system/core/rootdir/init.rc
 * - https://cs.android.com/android/platform/superproject/+/android-14.0.0_r1:packages/modules/adb/daemon/shell_service.cpp;l=379
 */
#define PROCESS_CONTEXT__SHELL "u:r:shell:s0"

/**
 * Get selinux context of the current process from the
 * `ENV__TERMUX__SE_PROCESS_CONTEXT` env variable.
 *
 * @return Return `true` if a valid selinux context was set in env
 * variable and was copied to the `buffer`, otherwise `false`.
 */
bool getSeProcessContextFromEnv(const char* logTag, const char* var, char buffer[], size_t bufferSize);

/**
 * Get selinux context of the current process from the
 * `/proc/self/attr/current` file.
 *
 * @return Return `true` if a valid selinux context was successfully read
 * and was copied to the `buffer`, otherwise `false`.
 */
bool getSeProcessContextFromFile(const char* logTag, char buffer[], size_t bufferSize);



#ifdef __cplusplus
}
#endif

#endif // LIBTERMUX_CORE__NOS__C__UNIX_SELINUX_UTILS___H
