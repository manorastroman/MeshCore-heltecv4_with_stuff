#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <helpers/CommonCLI.h>

class MyMesh;   // fwd decl — UITask reads live stats via MyMesh getters

class UITask {
  DisplayDriver* _display;
  MyMesh* _mesh;
  unsigned long _next_read, _next_refresh, _auto_off;
  int _prevBtnState;
  NodePrefs* _node_prefs;
  char _version_info[32];

  void renderCurrScreen();
  void renderStatusCard();   // rich layout for large (>=128px) panels
public:
  UITask(DisplayDriver& display) : _display(&display), _mesh(NULL) { _next_read = _next_refresh = 0; }
  void begin(MyMesh* mesh, NodePrefs* node_prefs, const char* build_date, const char* firmware_version);

  void loop();
};