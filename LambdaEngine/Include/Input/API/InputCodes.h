#pragma once
#include "LambdaEngine.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"


namespace LambdaEngine
{
	/*
	* EKey
	*/
	enum EKey : uint8
	{	
		KEY_UNKNOWN = 0,

		/* Numbers */
		KEY_0 = 7,
		KEY_1 = 8,
		KEY_2 = 9,
		KEY_3 = 10,
		KEY_4 = 11,
		KEY_5 = 12,
		KEY_6 = 13,
		KEY_7 = 14,
		KEY_8 = 15,
		KEY_9 = 16,
		
		/* Letters */
		KEY_A = 19,
		KEY_B = 20,
		KEY_C = 21,
		KEY_D = 22,
		KEY_E = 23,
		KEY_F = 24,
		KEY_G = 25,
		KEY_H = 26,
		KEY_I = 27,
		KEY_J = 28,
		KEY_K = 29,
		KEY_L = 30,
		KEY_M = 31,
		KEY_N = 32,
		KEY_O = 33,
		KEY_P = 34,
		KEY_Q = 35,
		KEY_R = 36,
		KEY_S = 37,
		KEY_T = 38,
		KEY_U = 39,
		KEY_V = 40,
		KEY_W = 41,
		KEY_X = 42,
		KEY_Y = 43,
		KEY_Z = 44,
		
		/* Function keys */
		KEY_F1	= 70,
		KEY_F2	= 71,
		KEY_F3	= 72,
		KEY_F4	= 73,
		KEY_F5	= 74,
		KEY_F6	= 75,
		KEY_F7	= 76,
		KEY_F8	= 77,
		KEY_F9	= 78,
		KEY_F10	= 79,
		KEY_F11	= 80,
		KEY_F12	= 81,
		KEY_F13	= 82,
		KEY_F14	= 83,
		KEY_F15	= 84,
		KEY_F16	= 85,
		KEY_F17	= 86,
		KEY_F18	= 87,
		KEY_F19	= 88,
		KEY_F20	= 89,
		KEY_F21	= 90,
		KEY_F22	= 91,
		KEY_F23	= 92,
		KEY_F24	= 93,
		KEY_F25	= 94,

		/* Keypad */
		KEY_KEYPAD_0        = 95,
		KEY_KEYPAD_1        = 96,
		KEY_KEYPAD_2        = 97,
		KEY_KEYPAD_3        = 98,
		KEY_KEYPAD_4        = 99,
		KEY_KEYPAD_5        = 100,
		KEY_KEYPAD_6        = 101,
		KEY_KEYPAD_7        = 102,
		KEY_KEYPAD_8        = 103,
		KEY_KEYPAD_9        = 104,
		KEY_KEYPAD_DECIMAL  = 105,
		KEY_KEYPAD_DIVIDE   = 106,
		KEY_KEYPAD_MULTIPLY = 107,
		KEY_KEYPAD_SUBTRACT = 108,
		KEY_KEYPAD_ADD      = 109,
		KEY_KEYPAD_ENTER    = 110,
		KEY_KEYPAD_EQUAL    = 111,

		/* Ctrl, Shift, Alt, etc.*/
		KEY_LEFT_SHIFT	  = 112,
		KEY_LEFT_CONTROL  = 113,
		KEY_LEFT_ALT	  = 114,
		KEY_LEFT_SUPER	  = 115,
		KEY_RIGHT_SHIFT	  = 116,
		KEY_RIGHT_CONTROL = 117,
		KEY_RIGHT_ALT	  = 118,
		KEY_RIGHT_SUPER	  = 119,
		KEY_MENU		  = 120,

		/* Other */
		KEY_SPACE		  = 1,
		KEY_APOSTROPHE	  = 2,  	/* ' */
		KEY_COMMA		  = 3,  	/* , */
		KEY_MINUS		  = 4,  	/* - */
		KEY_PERIOD		  = 5,  	/* . */
		KEY_SLASH		  = 6,  	/* / */
		KEY_SEMICOLON	  = 17,  	/* ; */
		KEY_EQUAL		  = 18,  	/* = */
		KEY_LEFT_BRACKET  = 45,		/* [ */
		KEY_BACKSLASH	  = 46,		/* \ */
		KEY_RIGHT_BRACKET = 47,		/* ] */
		KEY_GRAVE_ACCENT  = 48,		/* ` */
		KEY_WORLD_1		  = 49,		/* non-US #1 */
		KEY_WORLD_2		  = 50,		/* non-US #2 */
		KEY_ESCAPE		  = 51,
		KEY_ENTER		  = 52,
		KEY_TAB			  = 53,
		KEY_BACKSPACE	  = 54,
		KEY_INSERT		  = 55,
		KEY_DELETE		  = 56,
		KEY_RIGHT		  = 57,
		KEY_LEFT		  = 58,
		KEY_DOWN		  = 59,
		KEY_UP			  = 60,
		KEY_PAGE_UP		  = 61,
		KEY_PAGE_DOWN	  = 62,
		KEY_HOME		  = 63,
		KEY_END			  = 64,
		KEY_CAPS_LOCK	  = 65,
		KEY_SCROLL_LOCK	  = 66,
		KEY_NUM_LOCK	  = 67,
		KEY_PRINT_SCREEN  = 68,
		KEY_PAUSE		  = 69,

		KEY_LAST    = KEY_MENU,
		KEY_COUNT   = KEY_LAST + 1
	};
	
	/*
	* FModifierFlag
	*/
	enum FModifierFlag
	{
		MODIFIER_FLAG_NONE       = 0,
		MODIFIER_FLAG_CTRL       = FLAG(1),
		MODIFIER_FLAG_ALT        = FLAG(2),
		MODIFIER_FLAG_SHIFT      = FLAG(3),
		MODIFIER_FLAG_CAPS_LOCK  = FLAG(4),
		MODIFIER_FLAG_SUPER      = FLAG(5),
		MODIFIER_FLAG_NUM_LOCK   = FLAG(6),
	};

	/*
	* EMouseButton
	*/
	enum EMouseButton : uint8
	{
		MOUSE_BUTTON_UNKNOWN		= 0,

		MOUSE_BUTTON_LEFT			= 1,
		MOUSE_BUTTON_RIGHT			= 2,
		MOUSE_BUTTON_MIDDLE			= 3,
		MOUSE_BUTTON_BACK			= 4,
		MOUSE_BUTTON_FORWARD		= 5,
		MOUSE_BUTTON_SCROLL_UP		= 6,
		MOUSE_BUTTON_SCROLL_DOWN	= 7,

		MOUSE_BUTTON_LAST   = MOUSE_BUTTON_SCROLL_DOWN,
		MOUSE_BUTTON_COUNT  = MOUSE_BUTTON_LAST + 1
	};

	/*
	* Helpers
	*/
	inline const char* KeyToString(EKey key)
	{
		switch (key)
		{
			case KEY_0:					return "0";
			case KEY_1:					return "1";
			case KEY_2:					return "2";
			case KEY_3:					return "3";
			case KEY_4:					return "4";
			case KEY_5:					return "5";
			case KEY_6:					return "6";
			case KEY_7:					return "7";
			case KEY_8:					return "8";
			case KEY_9:					return "9";
			case KEY_A:					return "A";
			case KEY_B:					return "B";
			case KEY_C:					return "C";
			case KEY_D:					return "D";
			case KEY_E:					return "E";
			case KEY_F:					return "F";
			case KEY_G:					return "G";
			case KEY_H:					return "H";
			case KEY_I:					return "I";
			case KEY_J:					return "J";
			case KEY_K:					return "K";
			case KEY_L:					return "L";
			case KEY_M:					return "M";
			case KEY_N:					return "N";
			case KEY_O:					return "O";
			case KEY_P:					return "P";
			case KEY_Q:					return "Q";
			case KEY_R:					return "R";
			case KEY_S:					return "S";
			case KEY_T:					return "T";
			case KEY_U:					return "U";
			case KEY_V:					return "V";
			case KEY_W:					return "W";
			case KEY_X:					return "X";
			case KEY_Y:					return "Y";
			case KEY_Z:					return "Z";
			case KEY_F1:				return "F1";
			case KEY_F2:				return "F2";
			case KEY_F3:				return "F3";
			case KEY_F4:				return "F4";
			case KEY_F5:				return "F5";
			case KEY_F6:				return "F6";
			case KEY_F7:				return "F7";
			case KEY_F8:				return "F8";
			case KEY_F9:				return "F9";
			case KEY_F10:				return "F10";
			case KEY_F11:				return "F11";
			case KEY_F12:				return "F12";
			case KEY_F13:				return "F13";
			case KEY_F14:				return "F14";
			case KEY_F15:				return "F15";
			case KEY_F16:				return "F16";
			case KEY_F17:				return "F17";
			case KEY_F18:				return "F18";
			case KEY_F19:				return "F19";
			case KEY_F20:				return "F20";
			case KEY_F21:				return "F21";
			case KEY_F22:				return "F22";
			case KEY_F23:				return "F23";
			case KEY_F24:				return "F24";
			case KEY_F25:				return "F25";
			case KEY_KEYPAD_0:			return "KEYPAD_0";
			case KEY_KEYPAD_1:			return "KEYPAD_1";
			case KEY_KEYPAD_2:			return "KEYPAD_2";
			case KEY_KEYPAD_3:			return "KEYPAD_3";
			case KEY_KEYPAD_4:			return "KEYPAD_4";
			case KEY_KEYPAD_5:			return "KEYPAD_5";
			case KEY_KEYPAD_6:			return "KEYPAD_6";
			case KEY_KEYPAD_7:			return "KEYPAD_7";
			case KEY_KEYPAD_8:			return "KEYPAD_8";
			case KEY_KEYPAD_9:			return "KEYPAD_9";
			case KEY_KEYPAD_DECIMAL:	return "KEYPAD_DECIMAL";
			case KEY_KEYPAD_DIVIDE:		return "KEYPAD_DIVIDE";
			case KEY_KEYPAD_MULTIPLY:	return "KEYPAD_MULTIPLY";
			case KEY_KEYPAD_SUBTRACT:	return "KEYPAD_SUBTRACT";
			case KEY_KEYPAD_ADD:		return "KEYPAD_ADD";
			case KEY_KEYPAD_ENTER:		return "KEYPAD_ENTER";
			case KEY_KEYPAD_EQUAL:		return "KEYPAD_EQUAL";
			case KEY_LEFT_SHIFT:		return "LEFT_SHIFT";
			case KEY_LEFT_CONTROL:		return "LEFT_CONTROL";
			case KEY_LEFT_ALT:			return "LEFT_ALT";
			case KEY_LEFT_SUPER:		return "LEFT_SUPER";
			case KEY_RIGHT_SHIFT:		return "RIGHT_SHIFT";
			case KEY_RIGHT_CONTROL:		return "RIGHT_CONTROL";
			case KEY_RIGHT_ALT:			return "RIGHT_ALT";
			case KEY_RIGHT_SUPER:		return "RIGHT_SUPER";
			case KEY_MENU:				return "MENU";
			case KEY_SPACE:				return "SPACE";
			case KEY_APOSTROPHE:		return "APOSTROPHE";
			case KEY_COMMA:				return "COMMA";
			case KEY_MINUS:				return "MINUS";
			case KEY_PERIOD:			return "PERIOD";
			case KEY_SLASH:				return "SLASH";
			case KEY_SEMICOLON:			return "SEMICOLON";
			case KEY_EQUAL:				return "EQUAL";
			case KEY_LEFT_BRACKET:		return "LEFT_BRACKET";
			case KEY_BACKSLASH:			return "BACKSLASH";
			case KEY_RIGHT_BRACKET:		return "RIGHT_BRACKET";
			case KEY_GRAVE_ACCENT:		return "GRAVE_ACCENT";
			case KEY_WORLD_1:			return "WORLD_1";
			case KEY_WORLD_2:			return "WORLD_2";
			case KEY_ESCAPE:			return "ESCAPE";
			case KEY_ENTER:				return "ENTER";
			case KEY_TAB:				return "TAB";
			case KEY_BACKSPACE:			return "BACKSPACE";
			case KEY_INSERT:			return "INSERT";
			case KEY_DELETE:			return "DELETE";
			case KEY_RIGHT:				return "RIGHT";
			case KEY_LEFT:				return "LEFT";
			case KEY_DOWN:				return "DOWN";
			case KEY_UP:				return "UP";
			case KEY_PAGE_UP:			return "PAGE_UP";
			case KEY_PAGE_DOWN:			return "PAGE_DOWN";
			case KEY_HOME:				return "HOME";
			case KEY_END:				return "END";
			case KEY_CAPS_LOCK:			return "CAPS_LOCK";
			case KEY_SCROLL_LOCK:		return "SCROLL_LOCK";
			case KEY_NUM_LOCK:			return "NUM_LOCK";
			case KEY_PRINT_SCREEN:		return "PRINT_SCREEN";
			case KEY_PAUSE:				return "PAUSE";
			default:					return "UNKNOWN";
		}
	}

	inline const EKey StringToKey(String str)
	{
		static const THashTable<String, EKey> keyMap = {
			{"0", EKey::KEY_0},
			{"1", EKey::KEY_1},
			{"2", EKey::KEY_2},
			{"3", EKey::KEY_3},
			{"4", EKey::KEY_4},
			{"5", EKey::KEY_5},
			{"6", EKey::KEY_6},
			{"7", EKey::KEY_7},
			{"8", EKey::KEY_8},
			{"9", EKey::KEY_9},
			{"A", EKey::KEY_A},
			{"B", EKey::KEY_B},
			{"C", EKey::KEY_C},
			{"D", EKey::KEY_D},
			{"E", EKey::KEY_E},
			{"F", EKey::KEY_F},
			{"G", EKey::KEY_G},
			{"H", EKey::KEY_H},
			{"I", EKey::KEY_I},
			{"J", EKey::KEY_J},
			{"K", EKey::KEY_K},
			{"L", EKey::KEY_L},
			{"M", EKey::KEY_M},
			{"N", EKey::KEY_N},
			{"O", EKey::KEY_O},
			{"P", EKey::KEY_P},
			{"Q", EKey::KEY_Q},
			{"R", EKey::KEY_R},
			{"S", EKey::KEY_S},
			{"T", EKey::KEY_T},
			{"U", EKey::KEY_U},
			{"V", EKey::KEY_V},
			{"W", EKey::KEY_W},
			{"X", EKey::KEY_X},
			{"Y", EKey::KEY_Y},
			{"Z" , EKey::KEY_Z },
			{"F1" , EKey::KEY_F1 },
			{"F2" , EKey::KEY_F2 },
			{"F3" , EKey::KEY_F3 },
			{"F4" , EKey::KEY_F4 },
			{"F5" , EKey::KEY_F5 },
			{"F6" , EKey::KEY_F6 },
			{"F7" , EKey::KEY_F7 },
			{"F8" , EKey::KEY_F8 },
			{"F9" , EKey::KEY_F9 },
			{"F10" , EKey::KEY_F10},
			{"F11" , EKey::KEY_F11},
			{"F12" , EKey::KEY_F12},
			{"F13" , EKey::KEY_F13},
			{"F14" , EKey::KEY_F14},
			{"F15" , EKey::KEY_F15},
			{"F16" , EKey::KEY_F16},
			{"F17" , EKey::KEY_F17},
			{"F18" , EKey::KEY_F18},
			{"F19" , EKey::KEY_F19},
			{"F20" , EKey::KEY_F20},
			{"F21" , EKey::KEY_F21},
			{"F22" , EKey::KEY_F22},
			{"F23" , EKey::KEY_F23},
			{"F24" , EKey::KEY_F24},
			{"F25" , EKey::KEY_F25},
			{"KEYPAD_0" ,EKey::KEY_KEYPAD_0},
			{"KEYPAD_1" ,EKey::KEY_KEYPAD_1},
			{"KEYPAD_2" ,EKey::KEY_KEYPAD_2},
			{"KEYPAD_3" ,EKey::KEY_KEYPAD_3},
			{"KEYPAD_4" ,EKey::KEY_KEYPAD_4},
			{"KEYPAD_5" ,EKey::KEY_KEYPAD_5},
			{"KEYPAD_6" ,EKey::KEY_KEYPAD_6},
			{"KEYPAD_7" ,EKey::KEY_KEYPAD_7},
			{"KEYPAD_8" ,EKey::KEY_KEYPAD_8},
			{"KEYPAD_9" ,EKey::KEY_KEYPAD_9},
			{"KEYPAD_DECIMAL" ,EKey::KEY_KEYPAD_DECIMAL},
			{"KEYPAD_DIVIDE" ,EKey::KEY_KEYPAD_DIVIDE },
			{"KEYPAD_MULTIPLY",EKey::KEY_KEYPAD_MULTIPLY},
			{"KEYPAD_SUBTRACT",EKey::KEY_KEYPAD_SUBTRACT},
			{"KEYPAD_ADD" ,EKey::KEY_KEYPAD_ADD},
			{"KEYPAD_ENTER" ,EKey::KEY_KEYPAD_ENTER},
			{"KEYPAD_EQUAL" ,EKey::KEY_KEYPAD_EQUAL},
			{"LEFT_SHIFT" ,EKey::KEY_LEFT_SHIFT},
			{"LEFT_CONTROL" ,EKey::KEY_LEFT_CONTROL},
			{"LEFT_ALT" ,EKey::KEY_LEFT_ALT},
			{"LEFT_SUPER" ,EKey::KEY_LEFT_SUPER},
			{"RIGHT_SHIFT" ,EKey::KEY_RIGHT_SHIFT},
			{"RIGHT_CONTROL" ,EKey::KEY_RIGHT_CONTROL},
			{"RIGHT_ALT" ,EKey::KEY_RIGHT_ALT},
			{"RIGHT_SUPER" ,EKey::KEY_RIGHT_SUPER},
			{"MENU" ,EKey::KEY_MENU},
			{"SPACE" ,EKey::KEY_SPACE},
			{"APOSTROPHE" ,EKey::KEY_APOSTROPHE},
			{"COMMA" ,EKey::KEY_COMMA},
			{"MINUS" ,EKey::KEY_MINUS},
			{"PERIOD" ,EKey::KEY_PERIOD},
			{"SLASH" ,EKey::KEY_SLASH},
			{"SEMICOLON" ,EKey::KEY_SEMICOLON},
			{"EQUAL" ,EKey::KEY_EQUAL},
			{"LEFT_BRACKET" ,EKey::KEY_LEFT_BRACKET},
			{"BACKSLASH" ,EKey::KEY_BACKSLASH},
			{"RIGHT_BRACKET" ,EKey::KEY_RIGHT_BRACKET},
			{"GRAVE_ACCENT" ,EKey::KEY_GRAVE_ACCENT},
			{"WORLD_1" ,EKey::KEY_WORLD_1},
			{"WORLD_2" ,EKey::KEY_WORLD_2},
			{"ESCAPE" ,EKey::KEY_ESCAPE},
			{"ENTER" ,EKey::KEY_ENTER},
			{"TAB" ,EKey::KEY_TAB},
			{"BACKSPACE" ,EKey::KEY_BACKSPACE},
			{"INSERT" ,EKey::KEY_INSERT},
			{"DELETE" ,EKey::KEY_DELETE},
			{"RIGHT" ,EKey::KEY_RIGHT},
			{"LEFT" ,EKey::KEY_LEFT},
			{"DOWN" ,EKey::KEY_DOWN},
			{"UP" ,EKey::KEY_UP},
			{"PAGE_UP" ,EKey::KEY_PAGE_UP},
			{"PAGE_DOWN" ,EKey::KEY_PAGE_DOWN},
			{"HOME" ,EKey::KEY_HOME},
			{"END" ,EKey::KEY_END},
			{"CAPS_LOCK" ,EKey::KEY_CAPS_LOCK},
			{"SCROLL_LOCK" ,EKey::KEY_SCROLL_LOCK},
			{"NUM_LOCK" ,EKey::KEY_NUM_LOCK},
			{"PRINT_SCREEN" ,EKey::KEY_PRINT_SCREEN},
			{"PAUSE" ,EKey::KEY_PAUSE},
		};

		auto itr = keyMap.find(str);
		if (itr != keyMap.end()) 
		{
			return itr->second;
		}

		return EKey::KEY_UNKNOWN;
	}

	inline const char* ButtonToString(EMouseButton Button)
	{
		switch (Button)
		{
		case MOUSE_BUTTON_LEFT:			return "MOUSE_LEFT";
		case MOUSE_BUTTON_RIGHT:		return "MOUSE_RIGHT";
		case MOUSE_BUTTON_MIDDLE:		return "MOUSE_MIDDLE";
		case MOUSE_BUTTON_BACK:			return "MOUSE_BACK";
		case MOUSE_BUTTON_FORWARD:		return "MOUSE_FORWARD";
		case MOUSE_BUTTON_SCROLL_UP:	return "MOUSE_SCROLL_UP";
		case MOUSE_BUTTON_SCROLL_DOWN:	return "MOUSE_SCROLL_DOWN";
		default:					return "UNKNOWN";
		}
	}

	inline EMouseButton StringToButton(String str)
	{
		static const THashTable<String, EMouseButton> mouseButtonMap = {
			{"MOUSE_LEFT", EMouseButton::MOUSE_BUTTON_LEFT},
			{"MOUSE_RIGHT", EMouseButton::MOUSE_BUTTON_RIGHT},
			{"MOUSE_MIDDLE", EMouseButton::MOUSE_BUTTON_MIDDLE},
			{"MOUSE_BACK", EMouseButton::MOUSE_BUTTON_BACK},
			{"MOUSE_FORWARD", EMouseButton::MOUSE_BUTTON_FORWARD},
			{"MOUSE_SCROLL_UP", EMouseButton::MOUSE_BUTTON_SCROLL_UP},
			{"MOUSE_SCROLL_DOWN", EMouseButton::MOUSE_BUTTON_SCROLL_DOWN},
		};

		auto itr = mouseButtonMap.find(str);
		if (itr != mouseButtonMap.end()) {
			return itr->second;
		}
		return EMouseButton::MOUSE_BUTTON_UNKNOWN;
	}
}
