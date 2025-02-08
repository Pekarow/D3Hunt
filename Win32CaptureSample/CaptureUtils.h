#pragma once
#include <string>
#include <vector>
#include <windows.h>

#define Z 0x5A
#define ENTER VK_RETURN
#define TAB VK_TAB
#define CTRL VK_CONTROL
#define ARROW_UP VK_UP
#define ARROW_DOWN VK_DOWN
#define ARROW_LEFT VK_LEFT
#define ARROW_RIGHT VK_RIGHT
#define CTRL_ARROW_UP {CTRL, ARROW_UP}
#define CTRL_ARROW_DOWN {CTRL, ARROW_DOWN}
#define CTRL_ARROW_LEFT {CTRL, ARROW_LEFT}
#define CTRL_ARROW_RIGHT {CTRL, ARROW_RIGHT}

void SendClick(RECT rect, HWND hwnd);
void SendKeys(std::vector<SHORT> keys);
void SendKeyPress(SHORT key);
void SendKeyRelease(SHORT key);
void writeText(std::string msg);
