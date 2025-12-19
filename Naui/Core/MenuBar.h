#pragma once
#include <string>

namespace Naui {

class MenuBar {
public:
	MenuBar();

	virtual void RenderMenuBar();
    void LayoutMenu();
    void LayoutPopup();

private:
    std::string currentLayout;
    char newLayoutName[64];
    bool openSaveAsPopup;
    bool popupFocusRequest;
};

}