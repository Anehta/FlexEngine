#pragma once

#include "InputTypes.hpp"
#include "Pair.hpp"

#include <map>

namespace flex
{
	class ICallbackMouseButton;
	class ICallbackMouseMoved;
	class ICallbackKeyEvent;

	/*
	* There are three main ways of retrieving input:
	*   1. Call GetKey(Down/Pressed) with a key code value
	*     - This is the fastest, but also least portable method. Good for debugging.
	*
	*   2. Call GetActionDown with an action type
	*     - This requires setting up an abstract representation of the input
	*       action, but allows for users to remap the key code to whatever they like
	*
	*    3. Register as a listener
	*      - This is the most involved method, but allows for full input binding
	*        and proper event consumption. This is the method all real game inputs
	*        should use.
	*/
	class InputManager
	{
	public:
		void Initialize();

		void Update();
		void PostUpdate();

		void UpdateGamepadState(i32 gamepadIndex, real axes[6], u8 buttons[15]);
		GamepadState& GetGamepadState(i32 gamepadIndex);

		i32 GetActionDown(Action action) const;
		bool GetActionPressed(Action action) const;
		bool GetActionReleased(Action action) const;
		real GetActionAxisValue(Action action) const;

		i32 GetKeyDown(KeyCode keyCode, bool bIgnoreImGui = false) const;
		bool GetKeyPressed(KeyCode keyCode, bool bIgnoreImGui = false) const;
		bool GetKeyReleased(KeyCode keyCode, bool bIgnoreImGui = false) const;

		bool IsGamepadButtonDown(i32 gamepadIndex, GamepadButton button) const;
		bool IsGamepadButtonPressed(i32 gamepadIndex, GamepadButton button) const;
		bool IsGamepadButtonReleased(i32 gamepadIndex, GamepadButton button) const;

		real GetPrevGamepadAxisValue(i32 gamepadIndex, GamepadAxis axis) const;
		real GetGamepadAxisValue(i32 gamepadIndex, GamepadAxis axis) const;
		// Axis-equivalent to button "press"
		bool HasGamepadAxisValueJustPassedThreshold(i32 gamepadIndex, GamepadAxis axis, real threshold) const;

		void CursorPosCallback(double x, double y);
		void MouseButtonCallback(MouseButton mouseButton, KeyAction action, i32 mods);
		void ScrollCallback(double xOffset, double yOffset);
		void KeyCallback(KeyCode keyCode, KeyAction action, i32 mods);
		void CharCallback(u32 character);

		bool DidMouseWrap() const;

		void SetMousePosition(glm::vec2 mousePos, bool bUpdatePreviousPos = true);
		glm::vec2 GetMousePosition() const;
		void ClearMouseMovement();
		glm::vec2 GetMouseMovement() const;
		void ClearMouseButton(MouseButton mouseButton);
		bool IsAnyMouseButtonDown(bool bIgnoreImGui = false) const;
		bool IsMouseButtonDown(MouseButton mouseButton) const;
		bool IsMouseButtonPressed(MouseButton mouseButton) const;
		bool IsMouseButtonReleased(MouseButton mouseButton) const;
		real GetVerticalScrollDistance() const;
		void ClearVerticalScrollDistance();

		// posNorm: normalized position of center of the rect [-1, 1] (y = 1 at top of screen)
		// sizeNorm: normalized size of the rect [0, 1]
		bool IsMouseHoveringRect(const glm::vec2& posNorm, const glm::vec2& sizeNorm);

		glm::vec2 GetMouseDragDistance(MouseButton mouseButton);
		void ClearMouseDragDistance(MouseButton mouseButton);

		void ClearAllInputs();
		void ClearMouseInput();
		void ClearKeyboadInput();
		void ClearGampadInput(i32 gamepadIndex);

		void BindMouseButtonCallback(ICallbackMouseButton* callback, i32 priority);
		void UnbindMouseButtonCallback(ICallbackMouseButton* callback);

		void BindMouseMovedCallback(ICallbackMouseMoved* callback, i32 priority);
		void UnbindMouseMovedCallback(ICallbackMouseMoved* callback);

		void BindKeyEventCallback(ICallbackKeyEvent* callback, i32 priority);
		void UnbindKeyEventCallback(ICallbackKeyEvent* callback);

		void DrawImGuiKeyMapper(bool* bOpen);

		static i32 s_JoystickDisconnected;

	private:
		void HandleRadialDeadZone(real* x, real* y);
		void LoadInputBindingsFromFile();
		void SaveInputBindingsToFile();

		static const i32 GAMEPAD_BUTTON_COUNT = (i32)GamepadButton::_NONE;
		static const i32 MOUSE_BUTTON_COUNT = (i32)MouseButton::_NONE;
		static const real MAX_JOYSTICK_ROTATION_SPEED;

		std::map<KeyCode, Key> m_Keys;

		// Contains one entry for each entry in the Action enum
		std::vector<InputBinding> m_InputBindings;
		std::vector<Pair<ICallbackMouseButton*, i32>> m_MouseButtonCallbacks;
		std::vector<Pair<ICallbackMouseMoved*, i32>> m_MouseMovedCallbacks;
		std::vector<Pair<ICallbackKeyEvent*, i32>> m_KeyEventCallbacks;

		u32 m_MouseButtonStates;
		u32 m_MouseButtonsPressed;
		u32 m_MouseButtonsReleased;
		MouseDrag m_MouseButtonDrags[MOUSE_BUTTON_COUNT];
		glm::vec2 m_MousePosition = { 0, 0 };
		glm::vec2 m_PrevMousePosition = { 0, 0 };
		real m_ScrollXOffset = 0;
		real m_ScrollYOffset = 0;

		bool m_bMouseWrapped = false;

		static const std::string s_InputBindingFilePath;

		GamepadState m_pGamepadStates[2];
		GamepadState m_GamepadStates[2];

	};
} // namespace flex
