#pragma once

// menu de configuracion in-game (config_menu.cpp): F1 lo abre y cierra
namespace cfgmenu {

void Draw(double fps);
void Toggle();
void Close();
bool Open();
// sin input del juego: menu abierto, o la ventana sin foco (el teclado global no debe colarse)
bool Blocked();

}  // namespace cfgmenu
