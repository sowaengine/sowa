#include "app.hxx"
#include "command/interfaces/command_palette.hxx"

void App::command_interface_char_callback(InputEventChar event) {
	// std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring wstr = {static_cast<wchar_t>(event.codepoint)};
	// std::string str = converter.to_bytes(wstr);

	if (nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		if (this->m_commandInterface->text_cursor == this->m_commandInterface->text.size())
			this->m_commandInterface->text += wstr;
		else
			this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor) + wstr + this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

		this->m_commandInterface->text_cursor += wstr.size();
	}
}

void App::command_interface_key_callback(InputEventKey event) {
	if (event.action == KEY_PRESSED && event.key == KEY_P && event.modifiers.control && event.modifiers.shift && nullptr == this->m_commandInterface) {
		SetCommandInterface(new CommandPaletteInterface);
	}

	if (event.action == KEY_PRESSED && event.key == KEY_ESCAPE) {
		if (nullptr != this->m_commandInterface) {
			SetCommandInterface(nullptr);
		} else {
			exit(0);
		}
	}

	if (event.action == KEY_PRESSED && event.key == KEY_DOWN && nullptr != this->m_commandInterface) {
		if (this->m_commandInterface->options.size() > 0) {
			this->m_commandInterface->currentIndex += 1;

			this->m_commandInterface->currentIndex %= this->m_commandInterface->options.size();
		}
	}

	if (event.action == KEY_PRESSED && event.key == KEY_UP && nullptr != this->m_commandInterface) {
		if (this->m_commandInterface->options.size() > 0) {
			this->m_commandInterface->currentIndex -= 1;

			if (this->m_commandInterface->currentIndex == -1)
				this->m_commandInterface->currentIndex = this->m_commandInterface->options.size() - 1;
		}
	}

	if (event.action == KEY_PRESSED && event.key == KEY_ENTER && nullptr != this->m_commandInterface) {
		CommandInterface *oldInterface = this->m_commandInterface;

		if (this->m_commandInterface->action) {
			this->m_commandInterface->action();
		}

		if (this->m_commandInterface->options.size() > static_cast<size_t>(this->m_commandInterface->currentIndex)) {
			std::function<void()> func = this->m_commandInterface->options[this->m_commandInterface->currentIndex].action;
			if (func)
				func();
		}

		// if user did not set command interface to another, delete it.
		if (oldInterface == this->m_commandInterface) {
			SetCommandInterface(nullptr);
		}
	}

	if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_BACKSPACE && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		if (this->m_commandInterface->text.size() > 0 && this->m_commandInterface->text_cursor != 0) {
			if (event.modifiers.control) {
				std::size_t index = std::string::npos;
				if (this->m_commandInterface->text[this->m_commandInterface->text_cursor - 1] == ' ') {
					index = this->m_commandInterface->text.find_last_not_of(' ', this->m_commandInterface->text_cursor - 1);

				} else {
					index = this->m_commandInterface->text.find_last_of(' ', this->m_commandInterface->text_cursor - 1);
				}
				if (index != std::string::npos && index + 1 < this->m_commandInterface->text.size()) {
					index += 1;
				}

				std::wstring remaining = this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

				if (index == std::string::npos) {
					// has no space
					this->m_commandInterface->text = remaining;
					this->m_commandInterface->text_cursor = 0;
				} else {
					this->m_commandInterface->text = this->m_commandInterface->text.substr(0, index) + remaining;
					this->m_commandInterface->text_cursor = index;
				}
			} else {
				if (this->m_commandInterface->text_cursor == this->m_commandInterface->text.size())
					this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text.size() - 1);
				else
					this->m_commandInterface->text = this->m_commandInterface->text.substr(0, this->m_commandInterface->text_cursor - 1) + this->m_commandInterface->text.substr(this->m_commandInterface->text_cursor);

				if (this->m_commandInterface->text_cursor > 0)
					this->m_commandInterface->text_cursor -= 1;
			}
		}
	}

	if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_RIGHT && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		if (this->m_commandInterface->text.size() > 0) {
			this->m_commandInterface->text_cursor += 1;

			if (this->m_commandInterface->text_cursor > this->m_commandInterface->text.size())
				this->m_commandInterface->text_cursor = this->m_commandInterface->text.size();
		}
	}

	if ((event.action == KEY_PRESSED || event.action == KEY_REPEAT) && event.key == KEY_LEFT && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		if (this->m_commandInterface->text_cursor > 0)
			this->m_commandInterface->text_cursor -= 1;
	}

	if (event.action == KEY_PRESSED && event.key == KEY_HOME && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		this->m_commandInterface->text_cursor = 0;
	}

	if (event.action == KEY_PRESSED && event.key == KEY_END && nullptr != this->m_commandInterface && this->m_commandInterface->text_input) {
		this->m_commandInterface->text_cursor = this->m_commandInterface->text.size();
	}
}
