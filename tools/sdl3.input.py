import re

SCANCODE_PATTERN = re.compile(r"SDL_SCANCODE_(\S+)\s+=\s+(\d+),")
KEYCODE_PATTERN = re.compile(r"SDLK_(\S+)\s+(0x[0-9a-fA-F]+)")
KEYMOD_PATTERN = re.compile(r"SDL_KMOD_(\S+)\s+(0x[0-9a-fA-F]+)")
MOUSEBUTTON_PATTERN = re.compile(r"SDL_BUTTON_(\S+)\s+(\d+)")
GAMEPADBUTTON_PATTERN = re.compile(r"\s+SDL_GAMEPAD_BUTTON_((?!LABEL)\S+)")
GAMEPADAXIS_PATTERN = re.compile(r"\s+SDL_GAMEPAD_AXIS_(\S+)")

def main() -> None:
    scancodes = None
    with open("/usr/include/SDL3/SDL_scancode.h") as file:
        text = file.read()
        scancodes = re.findall(SCANCODE_PATTERN, text)
    keycodes = None
    keymods = None
    with open("/usr/include/SDL3/SDL_keycode.h") as file:
        text = file.read()
        keycodes = re.findall(KEYCODE_PATTERN, text)
        keymods = re.findall(KEYMOD_PATTERN, text)
    mousebuttons = None
    with open("/usr/include/SDL3/SDL_mouse.h") as file:
        text = file.read()
        mousebuttons = re.findall(MOUSEBUTTON_PATTERN, text)
    gamepadbuttons = None
    gamepadaxes = None
    with open("/usr/include/SDL3/SDL_gamepad.h") as file:
        text = file.read()
        gamepadbuttons = re.findall(GAMEPADBUTTON_PATTERN, text)
        gamepadaxes = re.findall(GAMEPADAXIS_PATTERN, text)
    with open("out.hpp", "w") as file:
        file.write("#ifndef OUT_HPP\n#define OUT_HPP\n")
        file.write("enum class scancode {\n")
        for name, code in scancodes:
            file.write(f"  key_{name.lower()} = {code},\n")
        file.write("};\n")
        file.write("enum class keycode {\n")
        for name, code in keycodes:
            file.write(f"  key_{name.lower()} = {code},\n")
        file.write("};\n")
        file.write("namespace modifier {\n")
        for name, code in keymods:
            file.write(f"constexpr const std::uint64_t {name.lower()}{{{code}}};\n")
        file.write("}\n")
        file.write("enum class mouse_button {\n")
        for name, code in mousebuttons:
            file.write(f"  {name.lower()} = {code},\n")
        file.write("};\n")
        i = -1
        file.write("enum class gamepad_button {\n")
        for name in gamepadbuttons:
            file.write(f"  {name.lower()} = {i}\n")
            i += 1
        file.write("};\n")
        file.write("enum class gamepad_axis {\n")
        i = -1
        for name in gamepadaxes:
            file.write(f"  {name.lower()} = {i}\n")
            i += 1
        file.write("};\n")
        file.write("#endif")
if __name__ == "__main__":
    main()
