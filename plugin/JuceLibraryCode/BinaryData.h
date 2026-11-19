/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   background_png;
    const int            background_pngSize = 2903751;

    extern const char*   footswitch_down_png;
    const int            footswitch_down_pngSize = 14973;

    extern const char*   footswitch_up_png;
    const int            footswitch_up_pngSize = 16482;

    extern const char*   knob_knob;
    const int            knob_knobSize = 137075;

    extern const char*   knob_png;
    const int            knob_pngSize = 3757318;

    extern const char*   knob_1_fixed_png;
    const int            knob_1_fixed_pngSize = 31391;

    extern const char*   knob_1_movable_png;
    const int            knob_1_movable_pngSize = 14496;

    extern const char*   led_green_1_png;
    const int            led_green_1_pngSize = 10826;

    extern const char*   led_green_2_png;
    const int            led_green_2_pngSize = 10527;

    extern const char*   led_off_png;
    const int            led_off_pngSize = 6014;

    extern const char*   led_orange_1_png;
    const int            led_orange_1_pngSize = 10527;

    extern const char*   led_orange_2_png;
    const int            led_orange_2_pngSize = 10301;

    extern const char*   led_red_1_png;
    const int            led_red_1_pngSize = 10387;

    extern const char*   led_yellow_1_png;
    const int            led_yellow_1_pngSize = 10244;

    extern const char*   led_yellow_2_png;
    const int            led_yellow_2_pngSize = 10581;

    extern const char*   toggle_off_png;
    const int            toggle_off_pngSize = 16780;

    extern const char*   toggle_on_png;
    const int            toggle_on_pngSize = 16999;

    extern const char*   atan_approximations_py;
    const int            atan_approximations_pySize = 2326;

    extern const char*   erase_f0xx_cfg;
    const int            erase_f0xx_cfgSize = 1217;

    extern const char*   erase_f4xx_cfg;
    const int            erase_f4xx_cfgSize = 1217;

    extern const char*   erase_f10x_cfg;
    const int            erase_f10x_cfgSize = 1217;

    extern const char*   erase_f37x_cfg;
    const int            erase_f37x_cfgSize = 1217;

    extern const char*   erase_h7xx_cfg;
    const int            erase_h7xx_cfgSize = 1236;

    extern const char*   stm32loader_py;
    const int            stm32loader_pySize = 12779;

    extern const char*   merge_hex_awk;
    const int            merge_hex_awkSize = 199;

    extern const char*   __init___py;
    const int            __init___pySize = 0;

    extern const char*   resources_compiler_py;
    const int            resources_compiler_pySize = 9064;

    extern const char*   LICENSE;
    const int            LICENSESize = 2746;

    extern const char*   makefile_inc;
    const int            makefile_incSize = 17351;

    extern const char*   README_md;
    const int            README_mdSize = 138;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 30;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
