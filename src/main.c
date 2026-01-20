#include <clay.h>
#include <GLES3/gl3.h>
#define RGFW_WAYLAND
#define RGFW_OPENGL
#define RGFWDEF
#include <RGFW.h>
#define GLSL_VERSION "#version 330 core"
#include <clay_renderer_gles3.h>
#include <clay_renderer_gles3_loader_stb.h>

#include <stdio.h>
#include <stdlib.h>

#include "colors.h"
#include "irc.h"
#include "da.h"

unsigned char font[] = {
#embed "../resources/Roboto-Regular.ttf"
, 0
};

static inline Clay_Color color_alpha(Clay_Color c, uint8_t alpha) {
    c.a = alpha;
    return c;
}

#define ARRLEN(xs) (sizeof(xs) / sizeof(*(xs)))
const int font_size = 24;

typedef struct {
    char *data;
    size_t len, cap;
} TextInput;

Messages system_messages = {0};

Channels channels = {0};
int current_channel = -1;

enum {
    STATE_LOGIN,
    STATE_CHAT,
} state = STATE_LOGIN;
TextInput *current_input = {0};

TextInput username = {0}, server = {0}, password = {0};
TextInput the_message = {0};

int server_fd = -1;
int users_online = 0;

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s\n", errorData.errorText.chars);
}

void render_text_input(RGFW_window *win, Clay_SizingAxis width, TextInput *inp,
                       Clay_ElementId id, Clay_String text) {
    TextInput empty_inp = {0};
    if (inp == NULL)
        inp = &empty_inp;
    CLAY(id, {.layout = {.sizing = {width, CLAY_SIZING_FIXED(32)},
                         .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                         .padding.left = 10},
              .backgroundColor = CATPPUCCIN_SURFACE0,
              .cornerRadius = CLAY_CORNER_RADIUS(font_size / 2.)}) {
        if (Clay_Hovered()) {
            RGFW_window_setMouseStandard(win, RGFW_mouseIbeam);
            if (RGFW_window_isMouseDown(win, RGFW_mouseLeft)) {
                current_input = inp;
            }
        }
        Clay_String buf_text = {
            .chars = inp->data,
            .length = inp->len,
            .isStaticallyAllocated = false,
        };
        CLAY_TEXT((inp == current_input || inp->len > 0) ? buf_text : text,
                  CLAY_TEXT_CONFIG({.fontSize  = font_size,
                                    .textColor = color_alpha(CATPPUCCIN_TEXT, inp->len > 0 ? 255 : 128)}));
    }
}

bool render_button(RGFW_window *win, Clay_String text, Clay_SizingAxis width,
                   Clay_Color bgColor, Clay_Color bgColorHover,
                   Clay_Color fgColor) {
    bool clicked = false;
    CLAY_AUTO_ID({.layout = {.sizing = {width,
                                CLAY_SIZING_FIXED(32)},
                     .childAlignment = {.y = CLAY_ALIGN_Y_CENTER,
                                        .x = CLAY_ALIGN_X_CENTER}},
          .backgroundColor = Clay_Hovered() ? bgColorHover : bgColor,
          .cornerRadius = CLAY_CORNER_RADIUS(font_size / 2.)}) {
        if (Clay_Hovered()) {
            RGFW_window_setMouseStandard(win, RGFW_mousePointingHand);
            if (RGFW_window_isMouseDown(win, RGFW_mouseLeft)) {
                clicked = true;
            }
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontSize = font_size, .textColor = fgColor}));
    }
    return clicked;
}

void render_login(RGFW_window *win) {
    struct {
        TextInput *inp;
        Clay_ElementId id;
        Clay_String text;
    } inputs[] = {
        {&server,   CLAY_ID("Server"),   CLAY_STRING("Server IP...")},
        {&username, CLAY_ID("Username"), CLAY_STRING("Username...")},
        {&password, CLAY_ID("Password"), CLAY_STRING("Password...")},
    };

    RGFW_window_setMouseStandard(win, RGFW_mouseArrow);
    CLAY(CLAY_ID("LoginWindow"),
         {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                     .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .childGap = 16},
          .backgroundColor = CATPPUCCIN_BASE}) {
        for (size_t i = 0; i < ARRLEN(inputs); i++) {
            render_text_input(win, CLAY_SIZING_PERCENT(0.4), inputs[i].inp, inputs[i].id, inputs[i].text);
        }
        if (render_button(win, CLAY_STRING("Login"), CLAY_SIZING_PERCENT(0.4), CATPPUCCIN_PINK, color_alpha(CATPPUCCIN_PINK, 128), CATPPUCCIN_BASE)) {
            state = STATE_CHAT;
            da_append(server, '\0');
            da_append(username, '\0');
            irc_connect(server.data, username.data);
        }
    }
}

void render_chat(RGFW_window *win) {
    CLAY(CLAY_ID("ChattingWindow"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .backgroundColor = {30, 30, 46, 255}}) {
        CLAY(CLAY_ID("SideBar"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                                  .sizing = {.width = CLAY_SIZING_FIXED(300),
                                             .height = CLAY_SIZING_GROW(0)},
                                  .padding = CLAY_PADDING_ALL(16),
                                  .childGap = 16}, .backgroundColor = (Clay_Color)CATPPUCCIN_SURFACE0}) {
            for (size_t i = 0; i < channels.len; i++) {
                Clay_String str = {
                    .chars = channels.data[i].name.data,
                    .length = channels.data[i].name.len,
                    .isStaticallyAllocated = false,
                };
                if (render_button(win, str, CLAY_SIZING_GROW(0), CATPPUCCIN_SURFACE1, CATPPUCCIN_SURFACE2, CATPPUCCIN_TEXT)) {
                    current_channel = i;
                    irc_join_channel(channels.data[i].name.data);
                }
            }
        }
        CLAY(CLAY_ID("chat"),
             {.layout = {.childAlignment.y = CLAY_ALIGN_Y_BOTTOM,
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                         .padding = CLAY_PADDING_ALL(16)}}) {
            Messages *messages = current_channel == -1
                                     ? &system_messages
                                     : &channels.data[current_channel].messages;
            for (size_t i = 0; i < messages->len; i++) {
                Clay_String text = {
                    .chars = messages->data[i].text.data,
                    .length = messages->data[i].text.len,
                };
                CLAY_TEXT(text, CLAY_TEXT_CONFIG({.fontSize = font_size, .textColor = CATPPUCCIN_TEXT}));
            }
            CLAY_AUTO_ID({.layout.sizing.width = CLAY_SIZING_GROW(0)}) {
                render_text_input(win, CLAY_SIZING_GROW(0), &the_message,
                                  CLAY_ID("Textbox"),
                                  CLAY_STRING("Your message here..."));
                bool send_button = render_button(
                    win, CLAY_STRING(" Send "), CLAY_SIZING_FIT(0),
                    CATPPUCCIN_PINK, color_alpha(CATPPUCCIN_PINK, 128),
                    CATPPUCCIN_BASE);
                if (send_button && the_message.len > 0) {
                    da_append(the_message, '\0');
                    Message msg = {0};
                    msg.sender.data = username.data;
                    msg.sender.len  = username.len;
                    da_append_many(msg.text, the_message.data, the_message.len);
                    da_append(channels.data[current_channel].messages, msg);
                    irc_send_message(the_message.data, channels.data[current_channel].name.data);
                    the_message.len = 0;
                }
            }
        }
    }
}

void keyfunc(RGFW_window *win, RGFW_key key, u8 keyChar, RGFW_keymod keyMod,
             RGFW_bool repeat, RGFW_bool pressed) {
    if (!pressed)
        return;
    if (current_input == NULL)
        return;
    if (keyChar == 8 && current_input->len > 0)
        current_input->len--;
    if (31 > keyChar || keyChar > 127)
        return;
    da_append(*current_input, keyChar);
}

RGFW_window *init_rgfw(i32 w, i32 h) {
    RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 3;
    hints->minor = 3;
    RGFW_setGlobalHints_OpenGL(hints);
    RGFW_window *win = RGFW_createWindow(
        "toki", 0, 0, w, h,
        RGFW_windowCenter | RGFW_windowOpenGL
    );
    if (!win) exit(1);
    RGFW_setKeyCallback(keyfunc);
    RGFW_window_setExitKey(win, RGFW_escape);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    return win;
}

int main() {
    i32 w = 800, h = 600;
    RGFW_window *win = init_rgfw(w, h);

    // CLAY init
    uint32_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Context *clay_ctx = Clay_Initialize(arena, (Clay_Dimensions){w, h},
                    (Clay_ErrorHandler){HandleClayErrors});
    Gles3_Renderer gles3 = {0};
    gles3.clayMemory = (Clay_Arena){
        .capacity = totalMemorySize,
        .memory = (char *)malloc(totalMemorySize),
    };
    Stb_FontData stbFonts[MAX_FONTS] = {0};
    Clay_SetCurrentContext(clay_ctx);
    Clay_SetMeasureTextFunction(Stb_MeasureText, &stbFonts);
    Gles3_SetRenderTextFunction(&gles3, Stb_RenderText, &stbFonts);
    Gles3_Initialize(&gles3, 4096);
    if (!Stb_LoadFontMemory(&gles3.fontTextures[0], &stbFonts[0], font, sizeof(font), 24.0f, 1024, 1024))
        abort();
    // Clay_SetDebugModeEnabled(true);

    // mainloop
    RGFW_window_getSize(win, &w, &h);
    glViewport(0, 0, w, h);

    // XXX
    da_append_str(username, "kala_telo");
    da_append_str(server, "127.0.0.1");
    state = STATE_CHAT;
    irc_connect(server.data, username.data);
    while (!RGFW_window_shouldClose(win)) {
        RGFW_event event = { 0 };
        i32 x = 0, y = 0;
        while (RGFW_window_checkEvent(win, &event)) {
            switch (event.type) {
            case RGFW_windowResized: {
                RGFW_window_getSize(win, &w, &h);
                glViewport(0, 0, w, h);
            } break;
            case RGFW_quit:
                printf("exit\n");
                break;
            }
        }
        float scroll_x = 0, scroll_y = 0;
        bool mouse_pressed = RGFW_isMouseDown(RGFW_mouseLeft);
        Clay_SetLayoutDimensions((Clay_Dimensions){w, h});
        RGFW_window_getMouse(win, &x, &y);
        RGFW_getMouseScroll(&scroll_x, &scroll_y);
        // TODO unhardcode fps
        Clay_UpdateScrollContainers(true, (Clay_Vector2) {scroll_x, scroll_y}, 1./60.);
        Clay_SetPointerState((Clay_Vector2){x, y}, mouse_pressed);
        Clay_BeginLayout();
        switch (state) {
        case STATE_LOGIN:
            render_login(win);
            break;
        case STATE_CHAT:
            render_chat(win);
            break;
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        Gles3_Render(&gles3, renderCommands, stbFonts);
        RGFW_window_swapBuffers_OpenGL(win);
        if (server_fd > 0) {
            irc_proccess();
        }
    }
    RGFW_window_close(win);
    irc_close();
    free(arena.memory);
    free(gles3.clayMemory.memory);
}
