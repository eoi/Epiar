/**\file			ui_lua.cpp
 * \author			Matt Zweig (thezweig@gmail.com)
 * \date			Created: Thursday, October 29, 2009
 * \date			Modified: Friday, November 14, 2009
 * \brief			Lua bridge for UI objects
 * \details
 */

#include "includes.h"
#include "UI/ui.h"
#include "ui_lua.h"
#include "ui_label.h"
#include "ui_window.h"
#include "ui_button.h"
#include "ui_picture.h"
#include "ui_slider.h"
#include "ui_tabs.h"
#include "Engine/models.h"

/** \class UI_Lua
 *  \brief Lua bridge for working with the UI.
 *  \todo Widgets are being stored as plain userdata pointers.
 *        This is very unsafe as it allows Lua to attempt to use deallocated memory.
 *  \todo The Widget interface should be better defined.
 *        There should be more utility functions to allow creative uses of the UI.
 */

void UI_Lua::RegisterUI(lua_State *L){
	// These Functions create new UI Elements
	// Call them like:
	// win = UI.newWindow( ... )
	static const luaL_Reg uiFunctions[] = {
		// Create Widgets
		{"newWindow", &UI_Lua::newWindow},
		{"newFrame", &UI_Lua::newFrame},
		{"newButton", &UI_Lua::newButton},
		{"newLabel", &UI_Lua::newLabel},
		{"newPicture", &UI_Lua::newPicture},
		{"newTextbox", &UI_Lua::newTextbox},
		{"newCheckbox", &UI_Lua::newCheckbox},
		{"newSlider", &UI_Lua::newSlider},
		{"newTabCont", &UI_Lua::newTabCont},
		{"newTab", &UI_Lua::newTab},
		{"add", &UI_Lua::addWidget},
		{"search", &UI_Lua::search},
		{NULL, NULL}
	};

	// These Functions inspect or modify UI Elements
	// Call them like:
	// win:add( ... )
	static const luaL_Reg uiMethods[] = {
		// Widget Getters
		{"IsChecked", &UI_Lua::IsChecked},
		{"GetText", &UI_Lua::GetText},
		{"GetEdges", &UI_Lua::GetEdges},

		// Widget Setters
		// Windowing Layout
		{"add", &UI_Lua::add},
		{"close", &UI_Lua::close},
		// Picture Modification
		{"rotatePicture", &UI_Lua::rotatePicture},
		{"setPicture", &UI_Lua::setPicture},
		{"setLuaClickCallback", &UI_Lua::setLuaClickCallback},
		// Label Modification
		{"setText", &UI_Lua::setText},
		// Checkbox Modification
		{"setChecked", &UI_Lua::setChecked},
		{"setSliderValue", &UI_Lua::setSliderValue},
		{NULL, NULL}
	};

	luaL_newmetatable(L, EPIAR_UI);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, uiMethods, 0);

	luaL_openlib(L, EPIAR_UI, uiFunctions, 0);

	lua_pop(L,2);
}

/** \brief Create a new Window
 *
 *  \note Unlike most UI_Lua Widget creators, this implicitely Adds the Window to the UI.
 *  \returns Lua userdata containing pointer to Window.
 */
int UI_Lua::newWindow(lua_State *L){
	int arg = 6;
	int n = lua_gettop(L);  // Number of arguments
	if (n < 5)
		return luaL_error(L, "Got %d arguments expected 5 (x, y, w, h, caption,[draggable], [ Widgets ... ])", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));
	string caption = luaL_checkstring (L, 5);

	// Allocate memory for a pointer to object
	Window **win = (Window**)lua_newuserdata(L, sizeof(Window**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);
	*win = new Window(x,y,w,h,caption);

	if( lua_isboolean(L, 6) ){
		bool draggable = lua_toboolean(L, 6);
		(*win)->SetDragability( draggable );
		arg = 7;
	}

	// Add this Window
	UI::Add(*win);

	// Collect 'extra' widgets and Add them as children
	for(; arg <= n;arg++){
		Widget** widget= (Widget**)lua_touserdata(L,arg);
		(*win)->AddChild(*widget);
	}
	
	return 1;
}

/** \brief Create a new Frame
 *
 *  \returns Lua userdata containing pointer to Frame.
 */
int UI_Lua::newFrame(lua_State *L){
	int arg;
	int n = lua_gettop(L);  // Number of arguments
	if (n < 4)
		return luaL_error(L, "Got %d arguments expected 4 (x, y, w, h, [ Widgets ... ])", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));

	// Allocate memory for a pointer to object
	Frame **frame = (Frame**)lua_newuserdata(L, sizeof(Frame**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);
	*frame = new Frame(x, y, w, h);

	// Collect 'extra' widgets and Add them as children
	for(arg=5; arg <= n;arg++){
		Widget** widget= (Widget**)lua_touserdata(L,arg);
		if( widget == NULL ) {
			return luaL_error(L, "argument %d to setLuaClickCallback is NULL", arg);
		}
		(*frame)->AddChild(*widget);
	}
	
	return 1;
}

/** \brief Close a Widget
 *
 *  \details When passed with no arguments, this will close all Widgets.
 *  When passed with one argument, it will treat
 */
int UI_Lua::close(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments

	if (n == 0) {
		UI::Close();
	}
	else if(n == 1) {
		Widget** widget = (Widget**)lua_touserdata(L,1);
		UI::Close(*widget);
	}
	else {
		luaL_error(L, "Got %d arguments expected 0 or 1 ([window])", n); 
	}
	return 0;
}

/** \brief Create a new Button
 *
 *  \returns Lua userdata containing pointer to Button.
 */
int UI_Lua::newButton(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if ( (n != 5) && (n != 6) )
		return luaL_error(L, "Got %d arguments expected 5 or 6 (x, y, w, h, caption, [Lua_code])", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));
	string caption = luaL_checkstring (L, 5);
	string code = "";
	if(n==6) code = luaL_checkstring (L, 6);

	// Allocate memory for a pointer to object
	Button **button= (Button**)lua_newuserdata(L, sizeof(Button**));
    luaL_getmetatable(L, EPIAR_UI);
    lua_setmetatable(L, -2);
	*button = new Button(x,y,w,h,caption,code);

	// Note: We're not putting this button anywhere!
	//       Lua will have to do that for us.
	//       This may be a bad idea (memory leaks from bad lua scripts)

	return 1;
}

/** \brief Create a new Slider
 *
 *  \returns Lua userdata containing pointer to Slider.
 */
int UI_Lua::newSlider(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if ( (n != 5) && (n != 6) && (n != 7) )
		return luaL_error(L,
		"Got %d arguments expected 5 or 6(x, y, w, h, label, [position], [callback] )", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));
	string label = luaL_checkstring (L, 5);
	float position = 0.5f;
	string callback = "";
	for(int index=6; index<=7; ++index )
	{
		if ( lua_isnumber(L, index) ) {
			position = TO_FLOAT( luaL_checknumber(L,  index) );
		} else if ( lua_isstring(L, index) ) {
			callback = luaL_checkstring(L,  index); 
			printf("Callback: %s\n", luaL_checkstring(L,  index) );
		}
	}

	// Allocate memory for a pointer to object
	Slider **slider= (Slider**)lua_newuserdata(L, sizeof(Slider**));
    luaL_getmetatable(L, EPIAR_UI);
    lua_setmetatable(L, -2);
	
	if (n == 7) {
		*slider = new Slider(x,y,w,h,label,position,callback);
	} else if (n == 6) {
		*slider = new Slider(x,y,w,h,label,position);
	} else {
		*slider = new Slider(x,y,w,h,label);
	}
	
	return 1;
}

/** \brief Create a new Tabs Widget
 *
 *  \returns Lua userdata containing pointer to Tabs Widget.
 */
int UI_Lua::newTabCont(lua_State *L){
	int n = lua_gettop(L);	// Number of arguments
	if ( (n != 5 ) )
		return luaL_error(L,
		"Got %d arguments expected 5 (x, y, w, h, label)", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));
	string name = luaL_checkstring (L, 5);
	Tabs **tabs = (Tabs**)lua_newuserdata(L, sizeof(Tabs**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);

	*tabs = new Tabs(x,y,w,h,name);

	return 1;
}

/** \brief Create a new Tab Widget
 *
 *  \returns Lua userdata containing pointer to Tab Widget.
 */
int UI_Lua::newTab(lua_State *L){
	int n = lua_gettop(L);	// Number of arguments
	if ( (n != 1 ) )
		return luaL_error(L,
		"Got %d arguments expected 1 (caption)", n);

	string name = luaL_checkstring (L, 1);
	
	Tab **tab = (Tab**)lua_newuserdata(L, sizeof(Tab**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);

	*tab = new Tab(name);

	return 1;
}

/** \brief Create a new Textbox
 *
 *  \returns Lua userdata containing pointer to Textbox.
 */
int UI_Lua::newTextbox(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if ( n < 4  )
		return luaL_error(L, "Got %d arguments expected 3, 4, or 5 (x, y, w, h, [text], [name])", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	int w = int(luaL_checknumber (L, 3));
	int h = int(luaL_checknumber (L, 4));
	string text = "";
	string name = "";
	if(n>=5) text = luaL_checkstring (L, 5);
	if(n>=6) name = luaL_checkstring (L, 6);

	// Allocate memory for a pointer to object
	Textbox **textbox = (Textbox**)lua_newuserdata(L, sizeof(Textbox**));
    luaL_getmetatable(L, EPIAR_UI);
    lua_setmetatable(L, -2);
	*textbox = new Textbox(x, y, w, h, text, name);

	return 1;
}

/** \brief Create a new Label
 *
 *  \returns Lua userdata containing pointer to Label.
 */
int UI_Lua::newLabel(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if ((n != 3) && (n != 4))
		return luaL_error(L, "Got %d arguments expected 3 or 4 (x, y, caption, [centered] )", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));
	string caption = luaL_checkstring (L, 3);

	// Allocate memory for a pointer to object
	Label **label= (Label**)lua_newuserdata(L, sizeof(Label**));
    luaL_getmetatable(L, EPIAR_UI);
    lua_setmetatable(L, -2);
	if(n==3){
		*label = new Label(x,y,caption);
	} else if(n==4) {
		bool centered = luaL_checknumber (L, 4) != 0.;
		*label = new Label(x,y,caption,centered);
	}

	return 1;
}

/** \brief Create a new Picture
 *
 *  \returns Lua userdata containing pointer to Picture.
 */
int UI_Lua::newPicture(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if ( (n != 3) && (n != 5) && (n != 9))
		return luaL_error(L, "Got %d arguments expected 3, 5, or 9 (x, y, [w, h,] modelname, [red,blue,green,alpha] )", n);

	int x = int(luaL_checknumber (L, 1));
	int y = int(luaL_checknumber (L, 2));

	int w, h;
	w = h = 0;
	if(n >= 5){
		w = int(luaL_checknumber (L, 3));
		h = int(luaL_checknumber (L, 4));
	}
	string picname = luaL_checkstring (L, (n >= 5 ? 5 : 3) );
	

	// Get Background Color
	float red,blue,green,alpha;
	red=blue=green=alpha=0.0f; // default is clear black
	if(n==9) {
		red   = float(luaL_checknumber (L, 6));
		blue  = float(luaL_checknumber (L, 7));
		green = float(luaL_checknumber (L, 8));
		alpha = float(luaL_checknumber (L, 9));
	}

	// Allocate memory for a pointer to object
	Picture **pic= (Picture**)lua_newuserdata(L, sizeof(Picture**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);
	if (w + h > 0)
		*pic = new Picture(x,y,w,h, picname );
	else
		*pic = new Picture(x,y, picname );
	(*pic)->SetColor(red,blue,green,alpha);

	return 1;
}

/** \brief Create a new Checkbox
 *
 *  \returns Lua userdata containing pointer to Checkbox.
 */
int UI_Lua::newCheckbox(lua_State *L) {
	int n = lua_gettop(L);  // Number of arguments
	if (n == 4){
		int x = int(luaL_checknumber (L, 1));
		int y = int(luaL_checknumber (L, 2));
		bool checked = luaL_checknumber(L, 3) != 0;
		string labeltext = luaL_checkstring (L, 4);
		
		Checkbox **checkbox = (Checkbox**)lua_newuserdata(L, sizeof(Checkbox**));
		luaL_getmetatable(L, EPIAR_UI);
		lua_setmetatable(L, -2);
		*checkbox = new Checkbox(x, y, checked, labeltext);
	} else {
		return luaL_error(L, "Got %d arguments expected 4 (x, y, chekced, labeltext)", n);
	}
	
	return 1;
}

/** \brief Add widgets to the UI.
 *
 *  \details This accepts multiple Widgets.
 */
int UI_Lua::addWidget(lua_State *L) {
	int n = lua_gettop(L);  // Number of arguments
	if (n == 0){
		return luaL_error(L, "Got %d arguments expected 1 (widgets, ...)", n);
	}

	for(int i=1; i<=n; i++){
		Widget** widget = (Widget**)lua_touserdata(L,i);
		UI::Add(*widget);
	}

	return 0;
}

/** \brief Add widgets to the UI.
 *
 *  \details This accepts multiple Widgets.
 *  \see UI::search, Container::search
 */
int UI_Lua::search(lua_State *L) {
	int n = lua_gettop(L);  // Number of arguments
	if (n != 1){
		return luaL_error(L, "Got %d arguments expected 1 (query)", n);
	}

	string query = luaL_checkstring (L, 1);
	Widget *result = UI::Search( query );
	if( result == NULL ) {
		LogMsg(WARN, "Failed to find a widget with the query '%s'.", query.c_str() );
		return 0;
	}

	Widget **passback = (Widget**)lua_newuserdata(L, sizeof(Widget**));
	luaL_getmetatable(L, EPIAR_UI);
	lua_setmetatable(L, -2);
	*passback = result;

	return 1;
}

/** \brief Change the Image in a Picture Widget
 *
 */
int UI_Lua::setPicture(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n == 2){
		Picture** pic = (Picture**)lua_touserdata(L,1);
		if( pic == NULL ) {
			return luaL_error(L, "first argument to setPicture is NULL");
		} else if( ((*pic)->GetMask() & WIDGET_PICTURE) == 0) {
			return luaL_error(L, "first argument to setPicture is not a Picture");
		}
		string picname = luaL_checkstring (L, 2);
		(*pic)->Set( picname );
	
	} else {
		luaL_error(L, "Got %d arguments expected 2 (self, picname)", n);
	}
	return 0;
}

/** \brief Set the Lua callback for clicks within pictures
 *  \todo, this functionality should be extended to non-Pictures.
 */
int UI_Lua::setLuaClickCallback(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n == 2){
		Picture** pic = (Picture**)lua_touserdata(L,1);
		if( pic == NULL ) {
			return luaL_error(L, "first argument to setLuaClickCallback is NULL");
		} else if( ((*pic)->GetMask() & WIDGET_PICTURE) == 0) {
			return luaL_error(L, "first argument to setLuaClickCallback is not a Picture");
		}
		string callback = luaL_checkstring (L, 2);
		(*pic)->SetLuaClickCallback( callback );
	
	} else {
		luaL_error(L, "Got %d arguments expected 2 (self, picname)", n);
	}
	return 0;
}

/** \brief add Widgets to a Container
 *  \details This accepts multiple Widgets.
 *
 */
int UI_Lua::add(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n >= 2){
		Container** ptrOuter = static_cast<Container**>(lua_touserdata(L,1));
		if( ptrOuter == NULL ) {
			return luaL_error(L, "first argument to add is NULL");
		} else if( ((*ptrOuter)->GetMask() & WIDGET_CONTAINER) == 0) {
			return luaL_error(L, "first argument to add is not a Container");
		}
		for(int i=2; i<=n; i++){
			Widget** ptrInner = (Widget**)lua_touserdata(L,i);
			if( ptrInner == NULL ) {
				return luaL_error(L, "argument %d to add is NULL", i);
			}
			(*ptrOuter)->AddChild(*ptrInner);
		}
	} else {
		luaL_error(L, "Got %d arguments expected 2 or more (self, widget [...])", n);
	}
	return 0;
}

/** \brief Resize a Widget
 *
 */
int UI_Lua::move(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 5)
		return luaL_error(L, "Got %d arguments expected 1 (self, x,y,w,h)", n);
	
	// Get the new postition
	Widget** widget = (Widget**)lua_touserdata(L,1);
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	int w = luaL_checkinteger(L, 3);
	int h = luaL_checkinteger(L, 4);

	if( widget == NULL ) {
		return luaL_error(L, "Cannot move NULL Widget");
	}

	// Move the widget
	(*widget)->SetX( x );
	(*widget)->SetY( y );
	(*widget)->SetW( w );
	(*widget)->SetH( h );

	return 0;
}

/** \brief Rotate a Picture Widget
 *
 */
int UI_Lua::rotatePicture(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 2)
		return luaL_error(L, "Got %d arguments expected 2 (self, angle )", n);

	Picture **pic= (Picture**)lua_touserdata(L,1);
	double angle = luaL_checknumber (L, 2);

	if( pic == NULL ) {
		return luaL_error(L, "Argument 1 to rotatePicture is NULL");
	} else if( ((*pic)->GetMask() & WIDGET_PICTURE) == 0) {
		return luaL_error(L, "Argument 1 to rotatePicture is not a Picture");
	}

	(*pic)->Rotate(angle);

	return 1;
}

/** \brief Set Widget Text
 *  \todo This should support more widget types.
 */
int UI_Lua::setText(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 2)
		return luaL_error(L, "Got %d arguments expected 2 (self, text)", n);

	Widget **widget = (Widget**)lua_touserdata(L,1);
	string text = luaL_checkstring(L, 2);

	if( widget == NULL ) {
		return luaL_error(L, "Cannot setText to NULL Widget.");
	}

	int mask = (*widget)->GetMask();
	mask &= ~WIDGET_CONTAINER; // Turn off the Container flag.
	switch( mask ) {
		// These Widget types currently support setText
		case WIDGET_LABEL:
			((Label*)(*widget))->SetText( text );
			break;
		case WIDGET_TEXTBOX:
			((Textbox*)(*widget))->SetText( text );
			break;

		// TODO These Widget Types do not currently accept setText, but they should.
		case WIDGET_TAB:
		case WIDGET_WINDOW:
		case WIDGET_BUTTON:
			return luaL_error(L, "Epiar does not currently calling setText on Widgets of type '%s'.", (*widget)->GetType().c_str() );
			break;

		// These Widget Types can't accept setText.
		case WIDGET_TABS:
		case WIDGET_FRAME:
		case WIDGET_SLIDER:
		case WIDGET_PICTURE:
		case WIDGET_DROPDOWN:
		case WIDGET_CHECKBOX:
		case WIDGET_SCROLLBAR:
		case WIDGET_CONTAINER:
		default:
			return luaL_error(L, "Cannot setText to Widget of type '%s'.", (*widget)->GetType().c_str() );
	}

	return 0;
}

/** \brief Test if a checkbox is checked.
 *  \returns true if the Checkbox is checked.
 */
int UI_Lua::IsChecked(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 1)
		return luaL_error(L, "Got %d arguments expected 1 (self)", n);

	Checkbox **box= (Checkbox**)lua_touserdata(L,1);

	if( box == NULL ) {
		return luaL_error(L, "Argument to IsChecked is NULL");
	} else if( ((*box)->GetMask() & WIDGET_CHECKBOX) == 0) {
		return luaL_error(L, "Argument to IsChecked is not a Checkbox");
	}

	lua_pushboolean(L, (int) (*box)->IsChecked() );

	return 1;
}

/** \brief Check or Uncheck a checkbox
 */
int UI_Lua::setChecked(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 2)
		return luaL_error(L, "Got %d arguments expected 2 (self, value)", n);

	Checkbox **box= (Checkbox**)lua_touserdata(L,1);
	bool checked = lua_toboolean(L, 2) != 0;

	if( box == NULL ) {
		return luaL_error(L, "Argument to SetChecked is NULL");
	} else if( ((*box)->GetMask() & WIDGET_CHECKBOX) == 0) {
		return luaL_error(L, "Argument to SetChecked is not a Checkbox");
	}

	(*box)->Set(checked);

	return 0;
}

/** \brief Set a Slider Value
 */
int UI_Lua::setSliderValue(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 2)
		return luaL_error(L, "Got %d arguments expected 2 (self, value)", n);
	
	Slider **slide= (Slider**)lua_touserdata(L,1);
	float percent= (float)lua_tonumber(L, 2);

	if( slide == NULL ) {
		return luaL_error(L, "Argument to setSliderValue is NULL");
	} else if( ((*slide)->GetMask() & WIDGET_SLIDER) == 0) {
		return luaL_error(L, "Argument to setSliderValue is not a Slider");
	}

	(*slide)->SetVal(percent);
	
	return 0;
}

/** \brief Get the Text Value of a Widget
 *  \todo Currently this only supports Labels and Textboxes, but it shouls support more Widget types.
 *  \returns string or nil
 */
int UI_Lua::GetText(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 1)
		return luaL_error(L, "Got %d arguments expected 1 (self)", n);

	Widget **widget= (Widget**)lua_touserdata(L,1);

	if( widget == NULL ) {
		return luaL_error(L, "Cannot getText from NULL Widget.");
	}

	int mask = (*widget)->GetMask();
	mask &= ~WIDGET_CONTAINER; // Turn off the Container flag.
	switch( mask ) {
		// These Widget types currently support setText
		case WIDGET_LABEL:
			lua_pushstring(L, ((Label*)(*widget))->GetText().c_str() );
			return 1;
			break;
		case WIDGET_TEXTBOX:
			lua_pushstring(L, ((Textbox*)(*widget))->GetText().c_str() );
			return 1;
			break;

		// TODO These Widget Types do not currently accept setText, but they should.
		case WIDGET_TAB:
		case WIDGET_WINDOW:
		case WIDGET_BUTTON:
			return luaL_error(L, "Epiar does not currently calling getText on Widgets of type '%s'.", (*widget)->GetType().c_str() );
			break;

		// These Widget Types can't accept setText.
		case WIDGET_TABS:
		case WIDGET_FRAME:
		case WIDGET_SLIDER:
		case WIDGET_PICTURE:
		case WIDGET_DROPDOWN:
		case WIDGET_CHECKBOX:
		case WIDGET_SCROLLBAR:
		case WIDGET_CONTAINER:
		default:
			return luaL_error(L, "Cannot getText to Widget of type '%s'.", (*widget)->GetType().c_str() );
	}

	return 0;
}

int UI_Lua::GetEdges(lua_State *L){
	int n = lua_gettop(L);  // Number of arguments
	if (n != 1)
		return luaL_error(L, "Got %d arguments expected 1 (self)", n);

	Widget **box= (Widget**)lua_touserdata(L,1);
	lua_pushinteger(L, (*box)->GetX() );
	lua_pushinteger(L, (*box)->GetY() );
	lua_pushinteger(L, (*box)->GetW() );
	lua_pushinteger(L, (*box)->GetH() );

	return 4;
}

